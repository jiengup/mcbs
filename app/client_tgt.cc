#include <brpc/channel.h>
#include <gflags/gflags.h>
#include <mcbs_client.hpp>

DEFINE_int32(id, -1, "Client user ID number");
DEFINE_bool(with_attachment, false, "Sent data with attachment or byte field in protobuf");
DEFINE_string(server, "0.0.0.0:8000", "IP Address of server");
DEFINE_uint32(write_depth, 1, "Number of concurrent write requests");
DEFINE_int32(max_retry, 0, "Max retries(not including the first RPC)"); 
DEFINE_int32(timeout_ms, 500, "Timeout of a single RPC request.");
DEFINE_bool(is_async, false, "Send requests asynchronously");
DEFINE_bool(is_simulation, true, "Simulate replay");
DEFINE_uint64(tolarance_latency, 50000, "Tolarance latency in microseconds");

int main(int argc, char* argv[]) {
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    CHECK_NE(FLAGS_id, -1) << "Please specify user ID with --id";
    
    mcbs::ClientOption option;
    
    option.id = FLAGS_id;
    option.server = FLAGS_server;
    option.max_retry = FLAGS_max_retry;
    option.with_attachment = FLAGS_with_attachment;
    option.write_depth = FLAGS_write_depth;
    option.tolarance_latency = FLAGS_tolarance_latency;

    option.ch_option.max_retry = FLAGS_max_retry;
    option.ch_option.timeout_ms = FLAGS_timeout_ms;

    auto *client = mcbs::Client::GetInstance();
    if (client->Init(option) != mcbs::Success) {
        LOG(ERROR) << "Fail to initialize client";
        return -1;
    }

    client->Replay(FLAGS_is_async, FLAGS_is_simulation);

    LOG(INFO) << "Client is going to quit";
    client->Stop();
    return 0;
}
