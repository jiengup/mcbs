#include <gflags/gflags.h>

#include <mcbs_server.hpp>

DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_string(spdk_json_config_file,
              "/users/guntherX/mcbs/spdk-config/8_tiny_ftls.json",
              "SPDK JSON config file");

int main(int argc, char* argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

  mcbs::ServerOption option;
  option.port = FLAGS_port;
  option.spdk_config_file = FLAGS_spdk_json_config_file;
  auto* server = mcbs::Server::GetInstance();

  server->Init(option);
  server->Start();

  return 0;
}
