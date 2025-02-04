#ifndef ART_RTGUI_BASE_MENU_BUTTON_H_
#define ART_RTGUI_BASE_MENU_BUTTON_H_

#include <vector>

#include <glibmm/ustring.h>
#include <gtkmm/menu.h>
#include <gtkmm/menubutton.h>
#include <gtkmm/menuitem.h>

namespace art {
class MenuButton : public Gtk::MenuButton {
public:
  MenuButton() = default;
  MenuButton(const Glib::ustring &label,
             const std::vector<Gtk::MenuItem *> &items);

  void AddItem(Gtk::MenuItem *item) { menu_->append(*item); }

private:
  Gtk::Menu *menu_ = nullptr;
};
} // namespace art

#endif // ART_RTGUI_BASE_MENU_BUTTON_H_
