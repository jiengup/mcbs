#include <string>

namespace mcbs {
enum ReturnCode { Success = 0, InitError = -1, OOM = -2, IOError = -3 };

inline std::string ReturnCodeToString(ReturnCode code) {
  switch (code) {
  case Success:
    return "Success";
  case InitError:
    return "InitError";
  case OOM:
    return "OOM";
  case IOError:
    return "IOError";
  default:
    return "Unknown";
  }
}
} // namespace mcbs