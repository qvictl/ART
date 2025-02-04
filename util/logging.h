#ifndef ART_UTIL_LOGGING_H_
#define ART_UTIL_LOGGING_H_

#include "util/str_util.h"
#include <glibmm/ustring.h>

namespace art {

template <typename... Args> void Info(const char *fmt, Args... args) {
  g_message("%s", StrFormat(fmt, args...).c_str());
}
void Info(const Glib::ustring &fmt);

// NOTE(zoulu): Looks like the 'error' actually means "Fatal" in other logging
// system, we stick with the industry standard. (e.g. glog, spdlog)
template <typename... Args> void Error(const char *fmt, Args... args) {
  g_critical("%s", StrFormat(fmt, args...).c_str());
}
void Error(const Glib::ustring &fmt);

template <typename... Args> void Warning(const char *fmt, Args... args) {
  g_warning("%s", StrFormat(fmt, args...).c_str());
}
void Warning(const Glib::ustring &fmt);

template <typename... Args> void Fatal(const char *fmt, Args... args) {
  g_error("%s", StrFormat(fmt, args...).c_str());
}
void Fatal(const Glib::ustring &fmt);

template <typename... Args> void Debug(const char *fmt, Args... args) {
  g_debug("%s", StrFormat(fmt, args...).c_str());
}
void Debug(const Glib::ustring &fmt);

} // namespace art

#endif // ART_UTIL_LOGGING_H_
