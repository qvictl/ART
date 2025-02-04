#ifndef ART_UTIL_STATUS_H_
#define ART_UTIL_STATUS_H_

#include <cstdint>
#include <string>
#include <string_view>

#include "util/str_util.h"
#include <glib.h>

namespace art {
// Simple version of absl::Status, with error code and message
enum class StatusCode : uint8_t {
  kOk = 0,
  kInvalidArgument = 1,
  kNotFound = 2,
  kAlreadyExists = 3,
  kPermissionDenied = 4,
  kUnauthenticated = 5,
  kResourceExhausted = 6,
  kFailedPrecondition = 7,
  kAborted = 8,
  kOutOfRange = 9,
  kUnimplemented = 10,
  kInternal = 11,
  kUnavailable = 12,
  kDataLoss = 13,
};

std::string_view StatusCodeName(StatusCode code);

class Status {
public:
  explicit Status(const StatusCode code, const std::string &message)
      : code_(code), message_(message) {}

  bool ok() const { return code_ == StatusCode::kOk; }
  operator bool() const { return ok(); }

  StatusCode code() const { return code_; }
  std::string_view message() const { return message_; }

private:
  StatusCode code_;
  std::string message_;
};

inline Status OkStatus() { return Status(StatusCode::kOk, ""); }
inline Status InvalidArgumentError(const std::string &message) {
  return Status(StatusCode::kInvalidArgument, message);
}
inline Status NotFoundError(const std::string &message) {
  return Status(StatusCode::kNotFound, message);
}
inline Status AlreadyExistsError(const std::string &message) {
  return Status(StatusCode::kAlreadyExists, message);
}
inline Status PermissionDeniedError(const std::string &message) {
  return Status(StatusCode::kPermissionDenied, message);
}
inline Status UnauthenticatedError(const std::string &message) {
  return Status(StatusCode::kUnauthenticated, message);
}
inline Status ResourceExhaustedError(const std::string &message) {
  return Status(StatusCode::kResourceExhausted, message);
}
inline Status FailedPreconditionError(const std::string &message) {
  return Status(StatusCode::kFailedPrecondition, message);
}
inline Status AbortedError(const std::string &message) {
  return Status(StatusCode::kAborted, message);
}
inline Status OutOfRangeError(const std::string &message) {
  return Status(StatusCode::kOutOfRange, message);
}
inline Status UnimplementedError(const std::string &message) {
  return Status(StatusCode::kUnimplemented, message);
}
inline Status InternalError(const std::string &message) {
  return Status(StatusCode::kInternal, message);
}
inline Status UnavailableError(const std::string &message) {
  return Status(StatusCode::kUnavailable, message);
}
inline Status DataLossError(const std::string &message) {
  return Status(StatusCode::kDataLoss, message);
}

// Simple version of absl::StatusOr<T>
template <typename T> class StatusOr {
public:
  StatusOr(const T &value) : value_(value), status_(OkStatus()) {}
  StatusOr(T &&value) : value_(std::move(value)), status_(OkStatus()) {}
  StatusOr(const Status &status) : status_(status) {}
  StatusOr(Status &&status) : status_(std::move(status)) {}

  bool ok() const { return status_.ok(); }
  operator bool() const { return ok(); }
  const Status &status() const { return status_; }
  std::string ToString() const {
    if (!status_.ok()) {
      return StrFormat("%s: %s", StatusCodeName(status_.code()),
                       status_.message());
    }
    return "Ok";
  }

  // Use invalid statusor lead to undefined behavior
  const T &value() const { return value_; }
  T &value() { return value_; }
  const T &operator*() const { return value_; }
  T &operator*() { return value_; }
  const T *operator->() const { return &value_; }
  T *operator->() { return &value_; }
  T &value_or(T &&default_value) {
    if (ok()) {
      return value_;
    }
    return default_value;
  }
  const T &value_or(const T &default_value) const {
    if (ok()) {
      return value_;
    }
    return default_value;
  }

private:
  T value_;
  Status status_;
};

} // namespace art

#endif // ART_UTIL_STATUS_H_
