#include <cstdint>
#include <brpc/server.h>
#include "io_impl.hpp"

namespace mcbs {
struct ServerOption {
    uint32_t port;
    std::string spdk_config_file;
    brpc::ServerOptions brpcs_options;
};

class Server {
    public:
    static Server* GetInstance() {
        static Server instance;
        return &instance;
    }
    void Init(const ServerOption& option);
    void Start();
    protected:
    Server();
    virtual ~Server();
    private:
    Server(const Server&) = delete;
    Server operator =(const Server&) = delete;
    Server(Server &&) = delete;
    Server operator =(Server &&) = delete;

    brpc::Server server_;
    WriteIOServiceImpl write_io_service_impl_;
    ServerOption option_;
};
} // namespace mcbs