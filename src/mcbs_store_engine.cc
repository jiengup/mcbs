#include <butil/logging.h>
#include <spdk/event.h>

#include <mcbs_store_engine.hpp>

#include "butil/macros.h"
#include "spdk/bdev.h"
#include "spdk/env.h"
#include "spdk/ftl.h"

namespace mcbs {

using ftl_io_ptr = ftl_io *;

struct ftl_write_context {
  StoreEngine *store_engine;
  ftl_io_ptr ftl_io;
  iovec *iov;
  void *buf;
  struct {
    void (*cb)(void *);
    void *ctx;
  } owner;
};

static void ftl_writev_cb(void *ctx, int status) {
  auto write_ctx = static_cast<ftl_write_context *>(ctx);
  auto store_engine = write_ctx->store_engine;
  if (!status) {
    LOG(DEBUG) << "bdev io write success";
    store_engine->stat_.success_requests << 1;
    store_engine->stat_.success_traffic << write_ctx->iov[0].iov_len;
  } else {
    LOG(ERROR) << "bdev io write failed";
  }
  store_engine->CleanupWriteContext(write_ctx);
  write_ctx->owner.cb(write_ctx->owner.ctx);
}

static void bdev_writev_cb(struct spdk_bdev_io *bdev_io, bool success,
                           void *cb_arg) {
  spdk_bdev_free_io(bdev_io);
  auto write_ctx = static_cast<ftl_write_context *>(cb_arg);
  auto store_engine = write_ctx->store_engine;
  if (success) {
    LOG(DEBUG) << "bdev io write success";
    store_engine->stat_.success_requests << 1;
    store_engine->stat_.success_traffic << write_ctx->iov[0].iov_len;
  } else {
    LOG(ERROR) << "bdev io write failed";
  }
  store_engine->CleanupWriteContext(write_ctx);
  write_ctx->owner.cb(write_ctx->owner.ctx);
}

static void spdk_ftl_free_cb(void *cb_arg, int status) {
  LOG(INFO) << "SPDK FTL device freed";
}

void StoreEngine::CleanupWriteContext(void *write_ctx) {
  auto ctx = static_cast<ftl_write_context *>(write_ctx);
  spdk_dma_free(ctx->buf);
  spdk_dma_free(ctx->ftl_io);
  iovec_pool_->free(ctx->iov);
  write_context_pool_->free(ctx);
}

StoreEngine::StoreEngine(const std::string &name, spdk_ftl_dev *ftl_dev,
                         spdk_bdev_desc *bdev_desc)
    : engine_name_(name),
      ftl_dev_(ftl_dev),
      bdev_desc_(bdev_desc),
      stat_(name) {
  LOG(INFO) << "Initializing store engine: " << engine_name_;

  io_channel_ = spdk_ftl_get_io_channel(ftl_dev_);
  if (io_channel_ == nullptr) {
    LOG(ERROR) << "Fail to get SPDK IO channel";
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
    return;
  }

  bdev_io_channel_ = spdk_bdev_get_io_channel(bdev_desc_);
  if (bdev_io_channel_ == nullptr) {
    LOG(ERROR) << "Fail to get SPDK BDEV IO channel";
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
    return;
  }

  struct spdk_ftl_attrs attrs;
  spdk_ftl_dev_get_attrs(ftl_dev_, &attrs, sizeof(attrs));

  block_size_ = attrs.block_size;
  ASSERT_LOG(block_size_ == 4096, "Block size must be 4096");
  num_blocks_ = attrs.num_blocks;
  tsfr_size_ = attrs.optimum_io_size;

  write_context_pool_ = new MemoryPool();
  if (write_context_pool_ == nullptr) {
    LOG(ERROR) << "Fail to create write context pool";
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
    return;
  }
  ftl_io_pool_ = new MemoryPool();
  if (ftl_io_pool_ == nullptr) {
    LOG(ERROR) << "Fail to create ftl io pool";
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
    return;
  }
  iovec_pool_ = new MemoryPool();
  if (iovec_pool_ == nullptr) {
    LOG(ERROR) << "Fail to create iovec pool";
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
    return;
  }
}

StoreEngine::~StoreEngine() {
  if (ftl_dev_ != nullptr) {
    spdk_ftl_dev_free(ftl_dev_, spdk_ftl_free_cb, nullptr);
  }
  if (io_channel_ != nullptr) {
    spdk_put_io_channel(io_channel_);
  }
}

ReturnCode StoreEngine::WriteBlocks(void *buf, uint64_t offset_blocks,
                                    uint64_t num_blocks, void (*cb)(void *),
                                    void *cb_ctx) {
  if (!buf) {
    LOG(ERROR) << "Invalid buffer";
    exit(-1);
    return StoreEngineWriteError;
  }
  if (offset_blocks + num_blocks > num_blocks_) {
    LOG(ERROR) << "Write out of bound";
    return BdevIOOOB;
  }

  ftl_write_context *write_ctx = static_cast<ftl_write_context *>(
      write_context_pool_->allocate(sizeof(ftl_write_context)));

  write_ctx->store_engine = this;
  write_ctx->ftl_io =
      reinterpret_cast<ftl_io *>(spdk_dma_malloc(spdk_ftl_io_size(), 0, NULL));
  write_ctx->iov = static_cast<iovec *>(iovec_pool_->allocate(sizeof(iovec)));
  write_ctx->owner.cb = cb;
  write_ctx->owner.ctx = cb_ctx;
  write_ctx->buf = spdk_dma_malloc(num_blocks * block_size_, 0, NULL);

  if (!write_ctx->ftl_io || !write_ctx->iov) {
    LOG(ERROR) << "Fail to allocate memory";
    return StoreEngineWriteError;
  }
  if (!write_ctx->buf) {
    LOG(ERROR) << "Fail to allocate buffer";
    return StoreEngineWriteError;
  }

  // TOTO(xgj): optimize this memory copy
  memcpy(write_ctx->buf, buf, num_blocks * block_size_);

  CHECK(write_ctx->ftl_io);
  CHECK(write_ctx->iov);

  write_ctx->iov[0].iov_base = write_ctx->buf;
  write_ctx->iov[0].iov_len = num_blocks * block_size_;

  stat_.total_requests << 1;
  stat_.total_traffic << write_ctx->iov[0].iov_len;

  // auto rc =
  //     spdk_ftl_writev(ftl_dev_, write_ctx->ftl_io, io_channel_,
  //     offset_blocks,
  //                     num_blocks, write_ctx->iov, 1, ftl_writev_cb,
  //                     write_ctx);

  auto rc = spdk_bdev_writev_blocks(bdev_desc_, bdev_io_channel_,
                                    write_ctx->iov, 1, offset_blocks,
                                    num_blocks, bdev_writev_cb, write_ctx);
    
  usleep(1700);

  if (rc != 0) {
    ftl_io_pool_->free(write_ctx->ftl_io);
    iovec_pool_->free(write_ctx->iov);
    write_context_pool_->free(write_ctx);
    LOG(ERROR) << "Fail to write blocks: " << ReturnCodeToString(rc);
    return StoreEngineWriteError;
  }

  return Success;
}

}  // namespace mcbs
