#include <spdk/ftl.h>

#include <mcbs_mempool.hpp>
#include <mcbs_retcode.hpp>
#include <mcbs_stat.hpp>
#include <string>

namespace mcbs {
class StoreEngine {
 public:
  StoreEngine() = delete;
  StoreEngine(const std::string &name, spdk_ftl_dev *bdev_desc);
  virtual ~StoreEngine();

  inline bool IsReady() const {
    return ftl_dev_ != nullptr && io_channel_ != nullptr;
  }

  ReturnCode WriteBlocks(void *buf, uint64_t offset_blocks, uint64_t num_blocks,
                         void (*cb)(void *), void *cb_ctx);
  void CleanupWriteContext(void *write_ctx);
  inline uint64_t GetBlockSize() const { return block_size_; }

  StoreEngineStat stat_;

 private:
  StoreEngine(const StoreEngine &) = delete;
  StoreEngine operator=(const StoreEngine &) = delete;
  StoreEngine(StoreEngine &&) = delete;
  StoreEngine operator=(StoreEngine &&) = delete;

  std::string engine_name_;
  spdk_ftl_dev *ftl_dev_;
  spdk_io_channel *io_channel_;

  uint64_t block_size_;
  uint64_t num_blocks_;
  uint64_t tsfr_size_;

  MemoryPool *write_context_pool_;
  MemoryPool *ftl_io_pool_;
  MemoryPool *iovec_pool_;
};
}  // namespace mcbs