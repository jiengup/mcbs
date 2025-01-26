#include <mcbs_server.hpp>

namespace mcbs {
Server::Server() {
    if (server_.AddService(&write_io_service_impl_, 
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
    }
}

Server::~Server() {
    server_.Stop(0);
    server_.Join();
}

void Server::Start(const ServerOption& option) {
    butil::EndPoint point = butil::EndPoint(butil::IP_ANY, option.port);
    if (server_.Start(point, &option.brpcs_options) != 0) {
        LOG(ERROR) << "Fail to start Server";
    }

    server_.RunUntilAskedToQuit();
}
}