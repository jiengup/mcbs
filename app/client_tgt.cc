#include <brpc/channel.h>
#include <mcbs_client.hpp>

DEFINE_int32(id, -1, "Client user ID number");
DEFINE_bool(with_attachment, false, "Sent data with attachment or byte field in protobuf");
DEFINE_string(server, "0.0.0.0:8000", "IP Address of server");
DEFINE_int32(max_retry, 0, "Max retries(not including the first RPC)"); 

int main(int argc, char* argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    CHECK_NE(FLAGS_id, -1) << "Please specify user ID with --id";
    
    mcbs::ClientOption option;
    
    option.id = FLAGS_id;
    option.server = FLAGS_server;
    option.max_retry = FLAGS_max_retry;
    option.with_attachment = 
    option.ch_option.max_retry = FLAGS_max_retry;

    auto *client = mcbs::Client::GetInstance();
    if (client->Init(option) != mcbs::Success) {
        LOG(ERROR) << "Fail to initialize client";
        return -1;
    }

    client->SimulateReplay();

    LOG(INFO) << "Client is going to quit";
    client->Stop();
    return 0;
}
