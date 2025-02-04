#include "rtgui/base/menu_button.h"

namespace art {
MenuButton::MenuButton(const Glib::ustring &label,
                       const std::vector<Gtk::MenuItem *> &items) {
  set_label(label);
  set_direction(Gtk::ArrowType::ARROW_DOWN);
  menu_ = Gtk::make_managed<Gtk::Menu>();
  for (auto *item : items) {
    menu_->append(*item);
    item->show();
  }
  set_popup(*menu_);
}

} // namespace art
