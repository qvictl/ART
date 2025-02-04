#ifndef ART_UTIL_KEYBOARD_UTIL_H_
#define ART_UTIL_KEYBOARD_UTIL_H_

#include <utility>

#include <gdkmm/types.h>
#include <glib.h>
#include <glibmm/ustring.h>

#include "util/status.h"

namespace art {
StatusOr<std::pair<Gdk::ModifierType, guint>>
ParseShortcut(const Glib::ustring &shortcut);
}

#endif // ART_UTIL_KEYBOARD_UTIL_H_
