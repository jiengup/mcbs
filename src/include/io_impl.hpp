#include <brpc/server.h>

#include <mcbs_mempool.hpp>

#include "io.pb.h"

namespace mcbs {
class WriteIOServiceImpl : public WriteIOService {
 public:
  WriteIOServiceImpl() : async_write_context_pool_(new MemoryPool) {}
  virtual ~WriteIOServiceImpl() {
    if (async_write_context_pool_) delete async_write_context_pool_;
  }
  virtual void Write(google::protobuf::RpcController *cntl_base,
                     const WriteIORequest *request, WriteIOResponse *response,
                     google::protobuf::Closure *done);
  virtual void AsyncWrite(google::protobuf::RpcController *cntl_base,
                          const WriteIORequest *request,
                          WriteIOResponse *response,
                          google::protobuf::Closure *done);

 private:
  MemoryPool *async_write_context_pool_;
};
};  // namespace mcbs