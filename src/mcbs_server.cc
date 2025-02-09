#include <mcbs_server.hpp>
#include <spdk/event.h>

namespace mcbs {

void spdk_app_start_cb(void* ctx) {
    LOG(INFO) << "SPDK application started";
}


Server::Server() {
    if (server_.AddService(&write_io_service_impl_, 
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
    }
}

Server::~Server() {
    spdk_app_fini();

    server_.Stop(0);
    server_.Join();
}


void Server::Init(const ServerOption& option) {
    option_ = option;

    LOG(INFO) << "Server start to init SPDK application";

    spdk_app_opts spdk_opts = {};
    spdk_app_opts_init(&spdk_opts, sizeof(spdk_opts));
    
    spdk_opts.name = "mcbs";
    spdk_opts.rpc_addr = NULL;
    spdk_opts.json_config_file = option_.spdk_config_file.c_str();

    if (spdk_app_start(&spdk_opts, spdk_app_start_cb, nullptr) != 0) {
        LOG(ERROR) << "Fail to start SPDK application";
    }
}

void Server::Start() {
    butil::EndPoint point = butil::EndPoint(butil::IP_ANY, option_.port);
    if (server_.Start(point, &option_.brpcs_options) != 0) {
        LOG(ERROR) << "Fail to start Server";
    }

    server_.RunUntilAskedToQuit();
}
}