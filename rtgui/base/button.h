#ifndef ART_RTGUI_BASE_BUTTON_H_
#define ART_RTGUI_BASE_BUTTON_H_

#include <glibmm/ustring.h>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <sigc++/functors/slot.h>

namespace art {
// Create a buuton with an icon, label, shortcut, and callback.
// The icon can be nullptr, label and shortcut can be empty.
class Button : public Gtk::Button {
public:
  using ClickedCallback = sigc::slot<void>;
  Button(Gtk::Image *image, const Glib::ustring &label,
         const Glib::ustring &shortcut,
         const Glib::RefPtr<Gtk::AccelGroup> &accel_group,
         const ClickedCallback &callback);
};

} // namespace art

#endif // ART_RTGUI_BASE_BUTTON_H_