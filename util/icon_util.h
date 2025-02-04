#ifndef ART_UTIL_ICON_UTIL_H_
#define ART_UTIL_ICON_UTIL_H_
#include <unordered_map>

#include <gdkmm/pixbuf.h>
#include <giomm/icon.h>
#include <glibmm/refptr.h>
#include <gtkmm/icontheme.h>

namespace art {
Glib::RefPtr<Gio::Icon> IconForType(const std::string &mime_type);
Glib::RefPtr<Gio::Icon> IconForFile(const Glib::ustring &path);
Glib::RefPtr<Gio::Icon> IconForOpenedDirectory();
// @param size The size of the icon in pixels.
Glib::RefPtr<Gdk::Pixbuf> IconToPixbuf(Glib::RefPtr<Gio::Icon> icon, int size);

// As Icons are constants, we should avoid creating them multiple times. The
// best practice is to create a map of them and reuse them.
// <file mime type, pixbuf>
std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
CreateIconPixbufsForCommonDirectoryTypes(int size);

std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
CreateIconPixbufsForCommonFileTypes(int size);

Glib::RefPtr<Gdk::Pixbuf> LookupIconPixbufForFile(
    const std::string &path,
    const std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
        &icon_pixbufs);
} // namespace art

#endif // ART_UTIL_ICON_UTIL_H_
