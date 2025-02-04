#ifndef ART_UTIL_STR_UTIL_H_
#define ART_UTIL_STR_UTIL_H_

#include <cstdarg>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <glibmm/ustring.h>

namespace art {
std::string StrFormat(const char *format, ...);

// Helper function to convert std::string arguments to const char*
template <typename T> auto FormatArg(T arg) -> T { return arg; }

inline const char *FormatArg(const std::string &arg) { return arg.c_str(); }
inline const char *FormatArg(const Glib::ustring &arg) { return arg.c_str(); }

// Dispatcher function to call the non-template StrFormat
template <typename... Args>
std::string StrFormatImpl(const char *format, Args... args) {
  return StrFormat(format, FormatArg(args)...);
}

// Wrapper function to handle std::string arguments
template <typename... Args>
std::string StrFormat(const char *format, Args... args) {
  // Call the dispatcher function, which calls the non-template StrFormat
  return StrFormatImpl(format, args...);
}

template <typename T> std::string ToStrHelper(const T &value) {
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

// Simple version of absl::StrCat for C++11
template <typename T> void StrCatImpl(std::ostringstream &oss, const T &arg) {
  oss << arg;
}

template <typename T, typename... Args>
void StrCatImpl(std::ostringstream &oss, const T &arg, const Args &...args) {
  oss << arg;
  StrCatImpl(oss, args...);
}

template <typename... Args> std::string StrCat(const Args &...args) {
  std::ostringstream oss;
  StrCatImpl(oss, args...);
  return oss.str();
}

// NOTE(luzou): This simple implementation of StrCat is not as efficient as
// construct multiple temporary strings
template <typename Container>
std::string StrJoin(const Container &values, std::string_view delimiter) {
  std::string result;
  for (const auto &value : values) {
    if (!result.empty()) {
      result += delimiter;
    }
    result = StrCat(result, value);
  }
  return result;
}

template <typename Container>
std::string StrJoin(const Container &values, std::string_view delimiter,
                    std::function<void(std::string *out,
                                       const typename Container::value_type &)>
                        converter) {
  std::string result;
  for (const auto &value : values) {
    if (!result.empty()) {
      result += delimiter;
    }
    converter(&result, value);
  }
  return result;
}

template <typename T> void StrAppend(std::string *out, const T &value) {
  out->append(ToStrHelper(value));
}

std::vector<std::string_view>
StrSplit(std::string_view str, std::string_view delimiter, bool skip_empty);

bool StartsWith(const Glib::ustring &str, const Glib::ustring &prefix);
bool EndsWith(const Glib::ustring &str, const Glib::ustring &suffix);
} // namespace art
#endif // ART_UTIL_STR_UTIL_H_
