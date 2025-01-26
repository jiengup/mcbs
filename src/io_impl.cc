#include <io_impl.hpp>

namespace mcbs {
void WriteIOServiceImpl::Write(google::protobuf::RpcController *cntl_base,
                               const WriteIORequest *request,
                               WriteIOResponse *response,
                               google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);

  brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

  LOG(INFO) << "Received request[log_id=" << cntl->log_id() << "] from "
            << cntl->remote_side() << " to " << cntl->local_side() << ": "
            << request->uid() << " " << request->offset() << " "
            << request->offset();
  response->set_internal_retcode(0);
  response->set_uid(request->uid());
}

void WriteIOServiceImpl::AsyncWrite(google::protobuf::RpcController *cntl_base,
                                    const WriteIORequest *request,
                                    WriteIOResponse *response,
                                    google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);

  brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

  LOG(INFO) << "Received request[log_id=" << cntl->log_id() << "] from "
            << cntl->remote_side() << " to " << cntl->local_side() << ": "
            << request->uid() << " " << request->offset() << " "
            << request->offset();
  response->set_internal_retcode(0);
  response->set_uid(request->uid());
}
} // namespace mcbs