#include <brpc/channel.h>

#include <mcbs_retcode.hpp>
#include <mcbs_stat.hpp>

#include "bthread/mutex.h"
#include "bthread/types.h"
#include "butil/time.h"
#include "io.pb.h"

#define MiB (1024 * 1024)
#define MAX_USER_DATA_SIZE (4 * MiB)

namespace mcbs {
struct ClientOption {
  std::string server;
  uint32_t max_retry;
  uint32_t write_depth;
  uint32_t id;
  uint32_t thread_num;
  uint64_t tolarance_latency;
  bool with_attachment = false;
  brpc::ChannelOptions ch_option;
};

class Client {
 public:
  static Client *GetInstance() {
    static Client instance;
    return &instance;
  }

  ReturnCode Init(const ClientOption &option);
  bool IsInit() const { return is_init; }

  void Replay(const bool is_async = false, const bool is_simulation = true);
  void SimulateReplay(const bool is_async = false);
  ReturnCode WriteRequest(const uint64_t offset, const uint64_t size,
                          const bool with_attachment = false);
  ReturnCode AsyncWriteRequest(const uint64_t offset, const uint64_t size,
                               const bool with_attachment = false);
  uint64_t GetSimulateOffset();
  void RecordTraffic(const uint64_t reqcnt, const uint64_t offset);
  bool WithAttachment() const { return with_attachment_; }
  uint64_t GetIOPS() const { return stat_->latency_recorder.qps(); }
  // void Increase
  void ShowStat();

  void Stop();

 protected:
  Client();
  virtual ~Client();

 private:
  Client(const Client &) = delete;
  Client operator=(const Client &) = delete;
  Client(Client &&) = delete;
  Client operator=(Client &&) = delete;

  void AsyncWriteDone(brpc::Controller *cntl, WriteIORequest *request, WriteIOResponse *response);

  bool is_init = false;
  uint32_t id_;
  uint64_t log_id_;
  uint32_t write_depth_;

  uint32_t thread_num_;
  std::vector<bthread_t> async_senders_;
  // control the number of inflight requests
  bthread_sem_t async_sem_;

  bool with_attachment_;
  char *user_data_;
  brpc::Channel channel_;
  uint64_t tolarance_latency_;
  WriteIOService_Stub *stub_;

  ClientStat *stat_;
  bthread_t stat_printer_;

  bthread_mutex_t simulate_offset_mutex_;
  uint64_t simulate_offset_;

  bthread_mutex_t traffic_recorder_mutex_;
  uint64_t acc_iops_ = 0;
  uint64_t acc_bandwidth_ = 0;
  butil::Timer record_timer_;

  uint64_t iops_ = 0;
  uint64_t bandwidth_ = 0;

  bthread_mutex_t inflight_request_mutex_;
  uint64_t inflight_request_ = 0;
};
};  // namespace mcbs