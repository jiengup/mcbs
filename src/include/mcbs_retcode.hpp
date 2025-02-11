#include <string>

namespace mcbs {
enum ReturnCode {
  Success = 0,
  InitError = -1,
  OOM = -2,
  IOError = -3,
  BdevNotFound = -4,
  IOChannelError = -5,
  StoreEngineWriteError = -6,
  StoreEngineStartError = -7,
  BdevIOOOB = -8,
};

inline std::string ReturnCodeToString(int code) {
  ReturnCode rc = static_cast<ReturnCode>(code);
  return ReturnCodeToString(rc);
}

inline std::string ReturnCodeToString(ReturnCode code) {
  switch (code) {
    case Success:
      return "Success";
    case InitError:
      return "Init Error";
    case OOM:
      return "Out of Memory";
    case IOError:
      return "IO Error";
    case BdevNotFound:
      return "Bdev Not Found";
    case IOChannelError:
      return "IO Channel Error";
    case StoreEngineWriteError:
      return "Store Engine Write Error";
    case StoreEngineStartError:
      return "Store Engine Start Error";
    case BdevIOOOB:
      return "Bdev IO out of bound";
    default:
      return "Unknown";
  }
}
}  // namespace mcbs