#include <mcbs_client.hpp>

#include "brpc/callback.h"
#include "bthread/bthread.h"
#include "io.pb.h"

namespace mcbs {

static void *simulation_async_sender(void *arg) {
  auto client = static_cast<Client *>(arg);
  uint64_t offset = 0;
  uint64_t size = 4096;
  while (!brpc::IsAskedToQuit()) {
    auto rc = client->AsyncWriteRequest(offset, size, true);
    CHECK(rc == Success);
  }
  bthread_usleep(50000);
  return nullptr;
}

static void *stat_printer(void *arg) {
  auto client = static_cast<Client *>(arg);
  while (!brpc::IsAskedToQuit()) {
    client->ShowStat();
    sleep(1);
  }
  return nullptr;
}

Client::Client() {}
ReturnCode Client::Init(const ClientOption &option) {
  CHECK(!IsInit());

  id_ = option.id;
  write_depth_ = option.write_depth;
  with_attachment_ = option.with_attachment;
  tolarance_latency_ = option.tolarance_latency;
  thread_num_ = option.thread_num;

  async_senders_.resize(thread_num_);

  if (bthread_sem_init(&async_sem_, write_depth_)) {
    LOG(ERROR) << "Fail to initialize async semaphore";
    return InitError;
  }

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
  memset(user_data_, 'a' + id_, MAX_USER_DATA_SIZE);

  stat_ = new ClientStat("client" + std::to_string(id_));
  if (stat_ == nullptr) {
    LOG(ERROR) << "Fail to allocate client stat";
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

  stat_->total_request << 1;
  stat_->total_traffic << size;
  stat_->inflight_request << 1;

  stub_->Write(&cntl, &request, &response, nullptr);
  if (cntl.Failed()) {
    LOG(ERROR) << "User" << id_ << "Fail to send request" << log_id_ << "due to"
               << cntl.ErrorText();
    stat_->error_request << 1;
    return IOError;
  }

  auto latency = cntl.latency_us();
  stat_->latency_recorder << latency;
  if (latency < tolarance_latency_) {
    stat_->good_request << 1;
    stat_->good_traffic << size;
  }

  return Success;
}

ReturnCode Client::AsyncWriteRequest(const uint64_t offset, const uint64_t size,
                                     const bool with_attachment) {
  CHECK(IsInit());
  WriteIORequest request;
  brpc::Controller *cntl = new brpc::Controller();
  WriteIOResponse *response = new WriteIOResponse();

  cntl->set_log_id(log_id_++);
  request.set_uid(id_);
  // TODO(xgj): check if offset and size are out of bound
  request.set_offset(offset);
  request.set_size(size);
  request.set_with_attachment(with_attachment);

  if (with_attachment) {
    // attach size bytes of data to the request
    CHECK_LE(size, MAX_USER_DATA_SIZE);
    cntl->request_attachment().append(user_data_, size);
  } else {
    CHECK_LE(size, MAX_USER_DATA_SIZE);
    request.set_data(user_data_, size);
  }

  bthread_sem_wait(&async_sem_);

  stat_->total_request << 1;
  stat_->total_traffic << size;
  stat_->inflight_request << 1;

  stub_->AsyncWrite(
      cntl, &request, response,
      brpc::NewCallback(this, &Client::AsyncWriteDone, cntl, response));
  return Success;
}

void Client::AsyncWriteDone(brpc::Controller *cntl, WriteIOResponse *response) {
  std::unique_ptr<brpc::Controller> cntl_guard(cntl);
  std::unique_ptr<WriteIOResponse> response_guard(response);

  bthread_sem_post(&async_sem_);
  stat_->inflight_request << -1;

  if (cntl->Failed()) {
    stat_->error_request << 1;
    LOG(ERROR) << "User" << id_ << "Fail to send request" << cntl->log_id()
               << "due to" << cntl->ErrorText();
    return;
  }

  auto latency = cntl->latency_us();
  stat_->latency_recorder << latency;
  if (latency < tolarance_latency_) {
    stat_->good_request << 1;
    stat_->good_traffic << response->written_size();
  }

  LOG(DEBUG) << "User" << id_ << "Receive response from" << cntl->remote_side()
             << "latency=" << cntl->latency_us() << "us";
}

void Client::SimulateReplay(const bool is_async) {
  CHECK(IsInit());

  if (bthread_start_background(&stat_printer_, nullptr, stat_printer, this) != 0) {
    LOG(ERROR) << "Fail to start stat printer";
    return;
  }

  if (!is_async) {
    LOG(INFO) << "User" << id_ << "Start to simulate replay in sync mode";
    uint64_t offset = 0;
    uint64_t size = 4096;
    while (!brpc::IsAskedToQuit()) {
      auto rc = WriteRequest(offset, size, with_attachment_);
      CHECK(rc == Success);
    }
  } else {
    LOG(INFO) << "User" << id_ << "Start to simulate replay in async mode";
    for (auto &async_sender_ : async_senders_) {
      if (bthread_start_background(&async_sender_, nullptr,
                                  simulation_async_sender, this) != 0) {
        LOG(ERROR) << "Fail to start async sender";
        return;
      }
    }
    for (auto const &async_sender_ : async_senders_) {
      bthread_join(async_sender_, nullptr);
    }
  }
  bthread_join(stat_printer_, nullptr);
}

void Client::Replay(const bool is_async, const bool is_simulation) {
  CHECK(IsInit());
  if (is_simulation) {
    SimulateReplay(is_async);
  } else {
    // TODO(xgj): implement replay from trace
    CHECK(false) << "Not implemented";
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
  if (stat_ != nullptr) {
    delete stat_;
  }
  is_init = false;
}

Client::~Client() {
  if (IsInit()) {
    Stop();
  }
}

void Client::ShowStat() {
  CHECK(IsInit());
  LOG(INFO) << "User" << id_;
  stat_->ShowStat();
}
}  // namespace mcbs