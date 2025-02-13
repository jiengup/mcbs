#include <mcbs_server.hpp>

namespace mcbs {

void WriteIOServiceImpl::Write(google::protobuf::RpcController *cntl_base,
                               const WriteIORequest *request,
                               WriteIOResponse *response,
                               google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);

  brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

  LOG(DEBUG) << "Received request[log_id=" << cntl->log_id() << "] from "
            << cntl->remote_side() << " to " << cntl->local_side() << ": "
            << request->uid() << " " << request->offset() << " "
            << request->size()
            << "latency: " << cntl->latency_us() * 1.0 / 1000 << "ms";
  response->set_internal_retcode(0);
  response->set_uid(request->uid());

  // usleep(30000);

  // TODO(xgj): set proper written size
  response->set_written_size(request->size());
}

void WriteIOServiceImpl::AsyncWrite(google::protobuf::RpcController *cntl_base,
                                    const WriteIORequest *request,
                                    WriteIOResponse *response,
                                    google::protobuf::Closure *done) {
  brpc::ClosureGuard done_guard(done);

  brpc::Controller *cntl = static_cast<brpc::Controller *>(cntl_base);

  LOG(DEBUG) << "Received request[log_id=" << cntl->log_id() << "] from "
             << cntl->remote_side() << " to " << cntl->local_side() << ": "
             << request->uid() << " " << request->offset() << " "
             << request->size();
  
  if (async_write_context_pool_ == nullptr) {
    LOG(ERROR) << "Fail to allocate async write context";
    response->set_internal_retcode(-1);
    return;
  }

  auto async_write_context = static_cast<AsyncWriteContext *>(
      async_write_context_pool_->allocate(sizeof(AsyncWriteContext)));
    
  if (async_write_context == nullptr) {
    LOG(ERROR) << "Fail to allocate async write context";
    response->set_internal_retcode(OOM);
    return;
  }

  async_write_context->response = response;
  async_write_context->request = request;
  async_write_context->cntl = cntl;
  async_write_context->done = done;
  
  auto server = Server::GetInstance();
  // TODO(xgj): set proper written size
  server->WriteRequest(async_write_context);
  done_guard.release();
  response->set_written_size(request->size());
}
}  // namespace mcbs