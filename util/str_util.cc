#include "util/str_util.h"

namespace art {
std::string StrFormat(const char *format, ...) {
  va_list args;
  va_start(args, format);

  // Determine required size
  int size = std::vsnprintf(nullptr, 0, format, args);
  va_end(args);

  if (size < 0) {
    return ""; // Error handling
  }

  std::string result(size, '\0');

  va_start(args, format);
  std::vsnprintf(&result[0], result.size() + 1, format, args);
  va_end(args);

  return result;
}

bool StartsWith(const Glib::ustring &str, const Glib::ustring &prefix) {
  return str.length() >= prefix.length() &&
         std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool EndsWith(const Glib::ustring &str, const Glib::ustring &suffix) {
  return str.length() >= suffix.length() &&
         std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::vector<std::string_view>
StrSplit(std::string_view str, std::string_view delimiter, bool skip_empty) {
  std::vector<std::string_view> result;
  if (str.empty()) {
    return result; // Return empty vector if input string is empty
  }

  size_t start = 0;
  size_t end = str.find(delimiter);

  while (end != std::string_view::npos) {
    std::string_view token = str.substr(start, end - start);
    if (!(skip_empty && token.empty())) {
      result.push_back(token);
    }
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }

  // Add the last token
  std::string_view last_token = str.substr(start);
  if (!(skip_empty && last_token.empty())) {
    result.push_back(last_token);
  }

  return result;
}
} // namespace art