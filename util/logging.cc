#include "util/logging.h"

namespace art {

void Info(const Glib::ustring &fmt) { g_message("%s", fmt.c_str()); }
void Error(const Glib::ustring &fmt) { g_critical("%s", fmt.c_str()); }
void Warning(const Glib::ustring &fmt) { g_warning("%s", fmt.c_str()); }
void Fatal(const Glib::ustring &fmt) { g_error("%s", fmt.c_str()); }
void Debug(const Glib::ustring &fmt) { g_debug("%s", fmt.c_str()); }

} // namespace art
