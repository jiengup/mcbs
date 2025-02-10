#include <butil/logging.h>
#include <spdk/event.h>

#include <mcbs_retcode.hpp>
#include <mcbs_store_engine.hpp>

#include "spdk/bdev.h"

namespace mcbs {

StoreEngine::StoreEngine(const std::string& bdev_name,
                         spdk_bdev_desc* bdev_desc)
    : engine_name_(bdev_name), bdev_desc_(bdev_desc) {
  LOG(INFO) << "Initializing store engine: " << engine_name_;

  io_channel_ = spdk_bdev_get_io_channel(bdev_desc_);
  if (io_channel_ == nullptr) {
    LOG(ERROR) << "Fail to get SPDK IO channel";
    spdk_bdev_close(bdev_desc_);
    return;
  }

  auto bdev = spdk_bdev_desc_get_bdev(bdev_desc_);
  block_size_ = spdk_bdev_get_block_size(bdev);
  num_blocks_ = spdk_bdev_get_num_blocks(bdev);
  md_size_ = spdk_bdev_get_md_size(bdev);
}

StoreEngine::~StoreEngine() {
  if (bdev_desc_ != nullptr) {
    spdk_bdev_close(bdev_desc_);
  }
  if (io_channel_ != nullptr) {
    spdk_put_io_channel(io_channel_);
  }
}
}  // namespace mcbs