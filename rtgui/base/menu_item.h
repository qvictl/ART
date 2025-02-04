#ifndef ART_RTGUI_BASE_MENU_ITEM_H_
#define ART_RTGUI_BASE_MENU_ITEM_H_

#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/menuitem.h>
#include <sigc++/functors/slot.h>
#include <utility>
#include <vector>

namespace art {
struct MenuInfo {
  Glib::ustring label;
  Glib::ustring shortcut;
  sigc::slot<void> callback;
};

Gtk::MenuItem *
CreateManagedMenuItem(const MenuInfo &info,
                      const Glib::RefPtr<Gtk::AccelGroup> &accel_group);

std::vector<Gtk::MenuItem *>
CreateManagedMenuItems(const std::vector<MenuInfo> &infos,
                       const Glib::RefPtr<Gtk::AccelGroup> &accel_group);

Gtk::MenuItem *
CreateManagedMenuItem(const Glib::ustring &label, const Glib::ustring &shortcut,
                      const Glib::RefPtr<Gtk::AccelGroup> &accel_group,
                      const sigc::slot<void> &callback);
} // namespace art
#endif // ART_RTGUI_BASE_MENU_ITEM_H_
