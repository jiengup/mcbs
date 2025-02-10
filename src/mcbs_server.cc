#include <spdk/bdev.h>
#include <spdk/bdev_module.h>
#include <spdk/event.h>

#include <mcbs_server.hpp>
#include <memory>

#include "butil/logging.h"
#include "butil/macros.h"

namespace mcbs {

static void bdev_open_event_cb(enum spdk_bdev_event_type type,
                               struct spdk_bdev* bdev, void* event_ctx) {}

static void spdk_app_start_cb(void* ctx) {
  LOG(INFO) << "SPDK application started";

  auto server = static_cast<Server*>(ctx);
  // Must called here
  for (const auto& bdev_name : server->GetSPDKBdevNames()) {
    spdk_bdev_desc* bdev_desc;
    if (spdk_bdev_open_ext(bdev_name.c_str(), true, bdev_open_event_cb, nullptr,
                           &bdev_desc) != 0) {
      LOG(ERROR) << "Fail to open SPDK bdev: " << bdev_name;
      return;
    }
    auto rc = server->StartStoreEngine(bdev_name, bdev_desc);
    if (rc != Success) {
      LOG(ERROR) << "Fail to start store engine: " << bdev_name;
      return;
    }
  }
  server->SetSPDKStarted(true);
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
  spdk_app_stop(0);
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

  for (const auto& name : spdk_bdev_names_) {
  }

  return Success;
}

ReturnCode Server::StartStoreEngine(const std::string& bdev_name,
                                    spdk_bdev_desc* bdev_desc) {
  CHECK(store_engines_.find(bdev_name) == store_engines_.end())
      << "Duplicate store engine: " << bdev_name;

  store_engines_[bdev_name] =
      std::unique_ptr<StoreEngine>(new StoreEngine(bdev_name, bdev_desc));

  if (!store_engines_[bdev_name]->IsReady()) {
    LOG(ERROR) << "Fail to start store engine: " << bdev_name;
    return StoreEngineError;
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