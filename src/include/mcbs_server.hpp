#include <brpc/server.h>
#include <bthread/bthread.h>
#include <spdk/bdev_module.h>

#include <cstdint>
#include <mcbs_retcode.hpp>

#include "io_impl.hpp"

namespace mcbs {
struct ServerOption {
  uint32_t port;
  std::string spdk_config_file;
  std::string bdev_names;
  brpc::ServerOptions brpcs_options;
};

class Server {
 public:
  static Server* GetInstance() {
    static Server instance;
    return &instance;
  }
  ReturnCode Init(const ServerOption& option);

  inline ServerOption GetOption() const { return option_; }
  inline std::vector<std::string> GetSPDKBdevNames() const {
    return spdk_bdev_names_;
  }
  inline void SetSpdkBdevDesc(const std::string& bdev_name, spdk_bdev_desc* bdev_desc) {
    spdk_bdev_descs_[bdev_name] = bdev_desc;
  }
  inline spdk_bdev_desc* GetSPDKBdevDescByName(const std::string& bdev_name) {
    return spdk_bdev_descs_[bdev_name];
  }

  void Start();
  bool IsSPDKStarted() const { return spdk_started_; }
  void SetSPDKStarted(bool flag) { spdk_started_ = flag; }
  void CleanSPDKEnv();

 protected:
  Server();
  virtual ~Server();

 private:
  Server(const Server&) = delete;
  Server operator=(const Server&) = delete;
  Server(Server&&) = delete;
  Server operator=(Server&&) = delete;


  brpc::Server server_;
  WriteIOServiceImpl write_io_service_impl_;
  ServerOption option_;

  bthread_t spdk_app_thread_;
  bool spdk_started_ = false;

  std::vector<std::string> spdk_bdev_names_;
  std::map<std::string, spdk_bdev_desc*> spdk_bdev_descs_;
};
}  // namespace mcbs