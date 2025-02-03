#include "io.pb.h"
#include <brpc/channel.h>
#include <mcbs_retcode.hpp>

#define MiB (1024 * 1024)
#define MAX_USER_DATA_SIZE (4 * MiB)

namespace mcbs {
struct ClientOption {
  std::string server;
  uint32_t max_retry;
  uint32_t write_depth;
  uint32_t id;
  uint32_t thread_num;
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
  ReturnCode WriteRequest(const uint64_t offset, const uint64_t size,
                          const bool with_attachment = false);
  ReturnCode AsyncWriteRequest(const uint64_t offset, const uint64_t size,
                               const bool with_attachment = false);
  void Replay(const bool is_async = false);
  void SimulateReplay(const bool is_async = false);
  bool IsInit() const { return is_init; }
  void Stop();

protected:
  Client();
  virtual ~Client();

private:
  Client(const Client &) = delete;
  Client operator=(const Client &) = delete;
  Client(Client &&) = delete;
  Client operator=(Client &&) = delete;

  bool is_init = false;
  uint32_t id_;
  uint64_t log_id_;
  uint32_t write_depth_;
  uint32_t thread_num_;
  std::vector<bthread_t> sender_bths_;
  bool with_attachment_;
  char *user_data_;
  brpc::Channel channel_;
  WriteIOService_Stub *stub_;
};
}; // namespace mcbs