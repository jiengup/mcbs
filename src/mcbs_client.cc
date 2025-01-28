#include <mcbs_client.hpp>

namespace mcbs {
Client::Client() {}
ReturnCode Client::Init(const ClientOption &option) {
  CHECK(!IsInit());
  id_ = option.id;
  write_depth_ = option.write_depth;
  with_attachment_ = option.with_attachment;

  if (channel_.Init(option.server.c_str(), &option.ch_option) != 0) {
    LOG(ERROR) << "Fail to initialize channel";
    return InitError;
  }

  stub_ = new WriteIOService_Stub(&channel_);
  if (stub_ == nullptr) {
    LOG(ERROR) << "Fail to initialize stub";
    return InitError;
  }

  user_data_ = new char[MAX_USER_DATA_SIZE];
  if (user_data_ == nullptr) {
    LOG(ERROR) << "Fail to allocate user data";
    return InitError;
  }

  is_init = true;
  return Success;
}

ReturnCode Client::WriteRequest(const uint64_t offset, const uint64_t size,
                                const bool with_attachment) {
  CHECK(IsInit());
  brpc::Controller cntl;
  WriteIORequest request;
  WriteIOResponse response;

  cntl.set_log_id(log_id_++);
  request.set_uid(id_);
  // TODO(xgj): check if offset and size are out of bound
  request.set_offset(offset);
  request.set_size(size);
  request.set_with_attachment(with_attachment);

  if (with_attachment) {
    // attach size bytes of data to the request
    CHECK_LE(size, MAX_USER_DATA_SIZE);
    cntl.request_attachment().append(user_data_, size);
  } else {
    CHECK_LE(size, MAX_USER_DATA_SIZE);
    request.set_data(user_data_, size);
  }

  stub_->Write(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "User" << id_ << "Fail to send request" << log_id_ << "due to"
               << cntl.ErrorText();
    return IOError;
  }

  return Success;
}


void Client::SimulateReplay(const bool is_async) {
  CHECK(IsInit());
  while (!brpc::IsAskedToQuit()) {
    uint64_t offset = 0;
    uint64_t size = 4096;
    if (is_async) {
        // TODO(xgj): implement AsyncWriteRequest
        CHECK(0);
    } else {
      WriteRequest(offset, size, with_attachment_);
    }
  }
}

void Client::Stop() {
  brpc::AskToQuit();
  while (!brpc::IsAskedToQuit()) {
    usleep(100);
  }
  if (stub_ != nullptr) {
    delete stub_;
  }
  if (user_data_ != nullptr) {
    delete user_data_;
  }
  is_init = false;
}

Client::~Client() {
  if (IsInit()) {
    Stop();
  }
}
} // namespace mcbs