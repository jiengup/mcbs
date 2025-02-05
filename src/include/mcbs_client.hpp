#include <brpc/channel.h>

#include <mcbs_retcode.hpp>
#include <mcbs_stat.hpp>

#include "io.pb.h"

#define MiB (1024 * 1024)
#define MAX_USER_DATA_SIZE (4 * MiB)

namespace mcbs {
struct ClientOption {
  std::string server;
  uint32_t max_retry;
  uint32_t write_depth;
  uint32_t id;
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

  void AsyncWriteDone(brpc::Controller *cntl, WriteIOResponse *response);

  bool is_init = false;
  uint32_t id_;
  uint64_t log_id_;
  uint32_t write_depth_;
  bthread_t async_sender_;
  bthread_sem_t async_sem_;
  bool with_attachment_;
  char *user_data_;
  brpc::Channel channel_;
  uint64_t tolarance_latency_;
  WriteIOService_Stub *stub_;

  ClientStat *stat_;
  bthread_t stat_printer_;
};
};  // namespace mcbs