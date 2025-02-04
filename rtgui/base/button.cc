#include "rtgui/base/button.h"

#include <algorithm>
#include <iostream>

#include <gtkmm/box.h>
#include <gtkmm/label.h>

#include "util/keyboard_util.h"

namespace art {
Button::Button(Gtk::Image *image, const Glib::ustring &label,
               const Glib::ustring &shortcut,
               const Glib::RefPtr<Gtk::AccelGroup> &accel_group,
               const ClickedCallback &callback) {
  auto *hbox =
      Gtk::make_managed<Gtk::Box>(Gtk::Orientation::ORIENTATION_HORIZONTAL, 5);
  if (image) {
    hbox->pack_start(*image, /*expand=*/false, /*fill=*/false);
  }
  hbox->pack_start(*Gtk::make_managed<Gtk::Label>(label), /*expand=*/true,
                   /*fill=*/true);
  add(*hbox);
  signal_clicked().connect(std::move(callback));

  if (!shortcut.empty()) {
    const auto ret = ParseShortcut(shortcut);
    if (!ret.ok()) {
      std::cerr << ret.ToString() << std::endl;
    } else {
      hbox->pack_start(*Gtk::make_managed<Gtk::Label>(shortcut),
                       /*expand=*/false,
                       /*fill=*/false);
      const auto &[mods, keyval] = ret.value();
      add_accelerator("clicked", accel_group, keyval, mods,
                      Gtk::AccelFlags::ACCEL_VISIBLE);
    }
  }
}

} // namespace art