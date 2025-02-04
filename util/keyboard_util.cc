#include "util/keyboard_util.h"

#include <gtkmm/accelgroup.h>

#include "util/str_util.h"

namespace art {
StatusOr<std::pair<Gdk::ModifierType, guint>>
ParseShortcut(const Glib::ustring &shortcut) {
  Gdk::ModifierType mods;
  guint keyval = 0;
  Gtk::AccelGroup::parse(shortcut, keyval, mods);
  if (keyval == 0) {
    return InvalidArgumentError(StrCat("Invalid key name: ", shortcut));
  }
  return std::make_pair(mods, keyval);
}
} // namespace art
