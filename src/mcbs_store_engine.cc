#include <butil/logging.h>
#include <spdk/event.h>

#include <mcbs_store_engine.hpp>

#include "butil/macros.h"
#include "spdk/bdev.h"

namespace mcbs {
struct ftl_write_context {
  StoreEngine *store_engine;
  ftl_io *ftl_io;
  iovec *iov;
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
    return;
  }
  store_engine->CleanupWriteContext(write_ctx);
  write_ctx->owner.cb(write_ctx->owner.ctx);
}

static void spdk_ftl_free_cb(void *cb_arg, int status) {
  LOG(INFO) << "SPDK FTL device freed";
}

void StoreEngine::CleanupWriteContext(void *write_ctx) {
  auto ctx = static_cast<ftl_write_context *>(write_ctx);
  ftl_io_pool_->free(ctx->ftl_io);
  iovec_pool_->free(ctx->iov);
  write_context_pool_->free(ctx);
}

StoreEngine::StoreEngine(const std::string &name, spdk_ftl_dev *ftl_dev)
    : engine_name_(name), ftl_dev_(ftl_dev), stat_(name) {
  LOG(INFO) << "Initializing store engine: " << engine_name_;

  io_channel_ = spdk_ftl_get_io_channel(ftl_dev_);
  if (io_channel_ == nullptr) {
    LOG(ERROR) << "Fail to get SPDK IO channel";
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
      static_cast<ftl_io *>(ftl_io_pool_->allocate(spdk_ftl_io_size()));
  write_ctx->iov = static_cast<iovec *>(iovec_pool_->allocate(sizeof(iovec)));
  write_ctx->owner.cb = cb;
  write_ctx->owner.ctx = cb_ctx;

  ASSERT_LOG(write_ctx->ftl_io != nullptr, "Fail to allocate ftl io");
  ASSERT_LOG(write_ctx->iov != nullptr, "Fail to allocate iovec");

  write_ctx->iov[0].iov_base = buf;
  write_ctx->iov[0].iov_len = num_blocks * block_size_;

  stat_.total_requests << 1;
  stat_.total_traffic << write_ctx->iov[0].iov_len;

  auto rc =
      spdk_ftl_writev(ftl_dev_, write_ctx->ftl_io, io_channel_, offset_blocks,
                      num_blocks, write_ctx->iov, 1, ftl_writev_cb, write_ctx);

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