#include "util/status.h"

namespace art {
std::string_view StatusCodeName(StatusCode code) {
  switch (code) {
  case StatusCode::kOk:
    return "kOk";
  case StatusCode::kInvalidArgument:
    return "kInvalidArgument";
  case StatusCode::kNotFound:
    return "kNotFound";
  case StatusCode::kAlreadyExists:
    return "kAlreadyExists";
  case StatusCode::kPermissionDenied:
    return "kPermissionDenied";
  case StatusCode::kUnauthenticated:
    return "kUnauthenticated";
  case StatusCode::kResourceExhausted:
    return "kResourceExhausted";
  case StatusCode::kFailedPrecondition:
    return "kFailedPrecondition";
  case StatusCode::kAborted:
    return "kAborted";
  case StatusCode::kOutOfRange:
    return "kOutOfRange";
  case StatusCode::kUnimplemented:
    return "kUnimplemented";
  case StatusCode::kInternal:
    return "kInternal";
  case StatusCode::kUnavailable:
    return "kUnavailable";
  case StatusCode::kDataLoss:
    return "kDataLoss";
  }
}

} // namespace art