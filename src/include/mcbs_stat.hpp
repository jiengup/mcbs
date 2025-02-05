#include <bvar/bvar.h>

#include "bvar/window.h"

namespace mcbs {
class ClientStat {
 public:
  // Increse when send request
  bvar::Adder<uint64_t> total_request;
  // Increse when receive response
  bvar::Adder<uint64_t> error_request;
  bvar::Adder<uint64_t> good_request;
  // User QPS & lantency
  // Record when receive response
  bvar::LatencyRecorder latency_recorder;
  bvar::Adder<uint64_t> total_traffic;
  bvar::Adder<uint64_t> good_traffic;
  bvar::PerSecond<bvar::Adder<uint64_t> > total_bandwidth;
  bvar::PerSecond<bvar::Adder<uint64_t> > good_bandwidth;
  ClientStat(std::string name)
      : total_request("mcbs", name + "total_request"),
        error_request("mcbs", name + "error_reqeust"),
        good_request("mcbs", name + "good_request"),
        latency_recorder("mcbs", name + "latency_recorder"),
        total_traffic("mcbs", name + "total_traffic"),
        good_traffic("mcbs", name + "good_traffic"),
        total_bandwidth("mcbs", name + "total_bandwidth", &total_traffic, 1),
        good_bandwidth("mcbs", name + "good_bandwidth", &good_traffic, 1) {}
  ~ClientStat() {}

  void ShowStat() const {
    LOG(INFO) << "[REQUESTS]" << "\n"
              << "Total request:" << total_request << "\n"
              << "Error request:" << error_request << "\n"
              << "Good request:" << good_request << "\n"
              << "[TRAFFIC]" << "\n"
              << "Total traffic:" << total_traffic << "\n"
              << "Good traffic:" << good_traffic << "\n"
              << "[BANDWIDTH]" << "\n"
              << "Total bandwidth:" << total_bandwidth << "\n"
              << "Good bandwidth:" << good_bandwidth << "\n"
              << "[LATENCY]" << "\n"
              << "Latency:" << latency_recorder.latency(1) << "\n"
              << "QPS:" << latency_recorder.qps(1);
  }
};

class ServerStat {
 public:
  bvar::Adder<uint64_t> total_requests;
  bvar::Adder<uint64_t> success_requests;
  bvar::Adder<uint64_t> failed_requsts;

  bvar::Adder<uint64_t> total_traffic;
  bvar::Adder<uint64_t> success_traffic;
  bvar::Adder<uint64_t> failed_traffic;

  bvar::PerSecond<bvar::Adder<uint64_t> > total_bandwidth;
  bvar::PerSecond<bvar::Adder<uint64_t> > success_bandwidth;

  ServerStat(std::string name)
      : total_requests("mcbs", name + "total_requests"),
        success_requests("mcbs", name + "success_requests"),
        failed_requsts("mcbs", name + "failed_requsts"),
        total_traffic("mcbs", name + "total_traffic"),
        success_traffic("mcbs", name + "success_traffic"),
        failed_traffic("mcbs", name + "failed_traffic"),
        total_bandwidth("mcbs", name + "total_bandwidth", &total_traffic, 1),
        success_bandwidth("mcbs", name + "success_bandwidth", &success_traffic,
                          1) {}
};
}  // namespace mcbs