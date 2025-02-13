#include <gflags/gflags.h>

#include <mcbs_server.hpp>

DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_string(spdk_json_config_file,
              "/users/guntherX/mcbs/spdk-config/ftl.json",
              "SPDK JSON config file");
DEFINE_string(bdev_names, "ftl0",
              "SPDK bdev names used as storage unit, split by comma");
DEFINE_string(ftl_algo, "single_group_greedy", "FTL streaming-divide algorithm");

int main(int argc, char* argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
  google::SetCommandLineOption("bthread_concurrency", "1");

  mcbs::ServerOption option;
  option.port = FLAGS_port;
  option.spdk_config_file = FLAGS_spdk_json_config_file;
  option.bdev_names = FLAGS_bdev_names;
  option.ftl_algo = FLAGS_ftl_algo;
  option.brpcs_options.max_concurrency = 4;
  auto* server = mcbs::Server::GetInstance();

  if (server->Init(option) != mcbs::Success) {
    LOG(ERROR) << "Fail to init server";
    return -1;
  }

  server->Start();

  return 0;
}
