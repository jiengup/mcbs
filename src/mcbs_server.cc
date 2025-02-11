#include <spdk/event.h>

#include <mcbs_server.hpp>
#include <memory>

#include "brpc/closure_guard.h"
#include "butil/logging.h"
#include "butil/macros.h"

namespace mcbs {
struct bpao_init_context {
  Server* server;
  std::string bpao_engine_name;
};

static void bpao_init_cb(struct spdk_ftl_dev* dev, void* cb_arg, int status) {
  auto ctx_guard = std::unique_ptr<bpao_init_context>(
      static_cast<bpao_init_context*>(cb_arg));
  auto server = ctx_guard->server;
  auto store_eng_name = ctx_guard->bpao_engine_name;
  if (status) {
    LOG(ERROR) << "Fail to init FTL device";
    spdk_app_stop(StoreEngineStartError);
    return;
  }

  if (server->StartStoreEngine(store_eng_name, dev) != Success) {
    LOG(ERROR) << "Fail to start store engine: " << store_eng_name;
    spdk_app_stop(StoreEngineStartError);
    return;
  }
}

static void spdk_app_start_cb(void* ctx) {
  LOG(INFO) << "SPDK application started";

  auto server = static_cast<Server*>(ctx);
  // Must called here
  for (const auto& bdev_name : server->GetSPDKBdevNames()) {
    auto store_eng_name = "bpao_over_" + bdev_name;
    spdk_ftl_conf conf;
    conf.name = const_cast<char*>(store_eng_name.c_str());
    conf.cache_bdev = const_cast<char*>(bdev_name.c_str());
    conf.mode |= SPDK_FTL_MODE_CREATE;
    auto init_ctx = new bpao_init_context{server, store_eng_name};
    if (spdk_ftl_dev_init(&conf, bpao_init_cb, ctx)) {
      LOG(ERROR) << "Fail to init FTL device: " << store_eng_name;
      spdk_app_stop(StoreEngineStartError);
      return;
    }
  }
  server->SetSPDKStarted(true);
}

static void async_write_cb(void *ctx) {
  auto async_ctx = static_cast<AsyncWriteContext*>(ctx);
  auto request = async_ctx->request;
  auto response = async_ctx->response;
  auto cntl = async_ctx->cntl;

  brpc::ClosureGuard done_guard(async_ctx->done);

  if (cntl->Failed()) {
    LOG(ERROR) << "Fail to write request: " << cntl->ErrorText();
    response->set_internal_retcode(IOError);
    return;
  }

  response->set_internal_retcode(0);
  response->set_uid(request->uid());
  response->set_written_size(request->size());
}

static void* spdk_starter(void* arg) {
  auto server = static_cast<Server*>(arg);
  auto option = server->GetOption();

  LOG(INFO) << "Server start to init SPDK application";

  spdk_app_opts spdk_opts = {};
  spdk_app_opts_init(&spdk_opts, sizeof(spdk_opts));

  spdk_opts.name = "mcbs";
  spdk_opts.rpc_addr = NULL;
  spdk_opts.json_config_file = option.spdk_config_file.c_str();

  int rc;

  rc =
      spdk_app_start(&spdk_opts, spdk_app_start_cb, static_cast<void*>(server));

  if (rc) {
    LOG(ERROR) << "SPDK application quit with error: "
               << ReturnCodeToString(rc);
    return nullptr;
  }

  return nullptr;
}

Server::Server() {
  if (server_.AddService(&write_io_service_impl_,
                         brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add service";
  }
}

void Server::CleanSPDKEnv() {
  if (IsSPDKStarted()) {
    spdk_app_stop(0);
  }
  bthread_join(spdk_app_thread_, nullptr);
  spdk_app_fini();
  LOG(INFO) << "SPDK application stopped";
  SetSPDKStarted(false);
}

Server::~Server() {
  for (auto& it : store_engines_) {
    if (it.second) {
      it.second.reset();
    }
    store_engines_.erase(it.first);
  }
  CleanSPDKEnv();
  CHECK(IsSPDKStarted() == true) << "SPDK is still running";
  server_.Stop(0);
  server_.Join();

}

ReturnCode Server::Init(const ServerOption& option) {
  option_ = option;

  std::stringstream names_ss(option_.bdev_names);
  std::string item_name;
  while (std::getline(names_ss, item_name, ',')) {
    spdk_bdev_names_.push_back(item_name);
  }

  if (bthread_start_background(&spdk_app_thread_, nullptr, spdk_starter,
                               this) != 0) {
    LOG(ERROR) << "Fail to start SPDK app";
    return InitError;
  }

  while (!IsSPDKStarted()) {
    usleep(100);
  }

  return Success;
}

ReturnCode Server::StartStoreEngine(const std::string& name,
                                    spdk_ftl_dev* ftl_dev) {
  CHECK(store_engines_.find(name) == store_engines_.end())
      << "Duplicate store engine: " << name;

  store_engines_[name] =
      std::unique_ptr<StoreEngine>(new StoreEngine(name, ftl_dev));

  if (!store_engines_[name]->IsReady()) {
    LOG(ERROR) << "Fail to start store engine: " << name;
    return StoreEngineStartError;
  }

  store_engine_id_map_[static_cast<int>(store_engine_id_map_.size())] = name;

  return Success;
}

ReturnCode Server::WriteRequest(AsyncWriteContext* ctx) {
  auto request = ctx->request;
  auto cntl = ctx->cntl;
  auto uid = request->uid();
  auto eng_idx = GetStoreEngineIndex(uid);
  auto eng_name = store_engine_id_map_[eng_idx];
  auto& store_engine = store_engines_[eng_name];
  if (store_engine == nullptr) {
    LOG(ERROR) << "Fail to find store engine: " << eng_name;
    return StoreEngineStartError;
  }

  ReturnCode rc;

  if (request->with_attachment()) {
    rc = store_engine->WriteBlocks(
        (void*)cntl->request_attachment().to_string().c_str(),
        request->offset(), request->size() / store_engine->GetBlockSize(),
        nullptr, ctx);
  } else {
    rc = store_engine->WriteBlocks(
        const_cast<char*>(request->data().c_str()), request->offset(),
        request->size() / store_engine->GetBlockSize(), nullptr, ctx);
  }

  if (rc) {
    LOG(ERROR) << "Fail to write request: " << ReturnCodeToString(rc);
    return rc;
  }
  return Success;
}

void Server::Start() {
  butil::EndPoint point = butil::EndPoint(butil::IP_ANY, option_.port);
  if (server_.Start(point, &option_.brpcs_options) != 0) {
    LOG(ERROR) << "Fail to start Server";
  }

  server_.RunUntilAskedToQuit();
}
}  // namespace mcbs