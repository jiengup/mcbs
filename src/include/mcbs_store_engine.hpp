#include <spdk/bdev.h>

#include <string>

namespace mcbs {
class StoreEngine {
 public:
  StoreEngine() = default;
  StoreEngine(const std::string &bdev_name, spdk_bdev_desc *bdev_desc);
  virtual ~StoreEngine();

  inline bool IsReady() const {
    return bdev_desc_ != nullptr && io_channel_ != nullptr;
  }

 private:
  StoreEngine(const StoreEngine &) = delete;
  StoreEngine operator=(const StoreEngine &) = delete;
  StoreEngine(StoreEngine &&) = delete;
  StoreEngine operator=(StoreEngine &&) = delete;

  std::string engine_name_;
  spdk_bdev_desc *bdev_desc_;
  spdk_io_channel *io_channel_;

  uint32_t block_size_;
  uint64_t num_blocks_;
  uint32_t md_size_;
};
}  // namespace mcbs