#include "rtgui/base/toolbar.h"

namespace art {
Toolbar::Toolbar(Gtk::Orientation orientation, int spacing)
    : Gtk::Box(orientation, spacing) {}

} // namespace art