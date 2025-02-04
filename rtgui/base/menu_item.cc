#include "rtgui/base/menu_item.h"

#include <iostream>

#include "util/keyboard_util.h"

namespace art {
Gtk::MenuItem *
CreateManagedMenuItem(const Glib::ustring &label, const Glib::ustring &shortcut,
                      const Glib::RefPtr<Gtk::AccelGroup> &accel_group,
                      const sigc::slot<void> &callback) {
  auto *item = Gtk::make_managed<Gtk::MenuItem>(label);
  item->signal_activate().connect(callback);
  if (!shortcut.empty()) {
    const auto ret = ParseShortcut(shortcut);
    if (!ret.ok()) {
      std::cerr << ret.ToString() << std::endl;
    } else {
      const auto &[mods, keyval] = ret.value();
      item->add_accelerator("activate", accel_group, keyval, mods,
                            Gtk::AccelFlags::ACCEL_VISIBLE);
    }
  }
  return item;
}

Gtk::MenuItem *
CreateManagedMenuItem(const MenuInfo &info,
                      const Glib::RefPtr<Gtk::AccelGroup> &accel_group) {
  return CreateManagedMenuItem(info.label, info.shortcut, accel_group,
                               info.callback);
}

std::vector<Gtk::MenuItem *>
CreateManagedMenuItems(const std::vector<MenuInfo> &infos,
                       const Glib::RefPtr<Gtk::AccelGroup> &accel_group) {
  std::vector<Gtk::MenuItem *> items;
  items.reserve(infos.size());
  for (const auto &info : infos) {
    items.push_back(CreateManagedMenuItem(info, accel_group));
  }
  return items;
}

} // namespace art