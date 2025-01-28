#include <gflags/gflags.h>
#include <mcbs_server.hpp>

DEFINE_int32(port, 8000, "TCP Port of this server");

int main(int argc, char* argv[]) {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    
    mcbs::ServerOption option;
    option.port = FLAGS_port;
    auto *server = mcbs::Server::GetInstance();

    server->Start(option);

    return 0;
}
