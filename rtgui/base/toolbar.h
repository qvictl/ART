#ifndef ART_RTGUI_BASE_TOOLBAR_H_
#define ART_RTGUI_BASE_TOOLBAR_H_

#include <gtkmm/box.h>

namespace art {
class Toolbar : public Gtk::Box {
public:
  Toolbar(Gtk::Orientation orientation, int spacing);
  // TODO(zoulu): Add more methods as needed.
};
} // namespace art
#endif // ART_RTGUI_BASE_TOOLBAR_H_
