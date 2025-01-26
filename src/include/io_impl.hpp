#include "io.pb.h"
#include <brpc/server.h>

namespace mcbs {
class WriteIOServiceImpl : public WriteIOService {
public:
  WriteIOServiceImpl() {}
  virtual ~WriteIOServiceImpl() {}
  virtual void Write(google::protobuf::RpcController *cntl_base,
                     const WriteIORequest *request, WriteIOResponse *response,
                     google::protobuf::Closure *done);
  virtual void AsyncWrite(google::protobuf::RpcController *cntl_base,
                          const WriteIORequest *request, WriteIOResponse *response,
                          google::protobuf::Closure *done);
};
}; // namespace mcbs