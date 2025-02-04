#include "util/icon_util.h"
#include "util/file_util.h"

#include <giomm.h>
#include <gtkmm/icontheme.h>
#include <iterator>

namespace art {

Glib::RefPtr<Gio::Icon> IconForType(const std::string &mime_type) {
  return Gio::content_type_get_icon(mime_type);
}

Glib::RefPtr<Gio::Icon> IconForFile(const Glib::ustring &path) {
  return IconForType(GetFileType(path));
}

Glib::RefPtr<Gio::Icon> IconForOpenedDirectory() {
  static const auto kFolderOpen = Gio::ThemedIcon::create("folder-open");
  return kFolderOpen;
}

Glib::RefPtr<Gdk::Pixbuf> IconToPixbuf(Glib::RefPtr<Gio::Icon> icon, int size) {
  Glib::RefPtr<Gtk::IconTheme> icon_theme = Gtk::IconTheme::get_default();
  const auto incon_info = icon_theme->lookup_icon(icon, size);
  if (!incon_info) {
    return {};
  }
  return incon_info.load_icon();
}

std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
CreateIconPixbufsForCommonDirectoryTypes(int size) {
  return {
      {"inode/directory",
       IconToPixbuf(Gio::ThemedIcon::create("folder"), size)},
      {"drive", IconToPixbuf(Gio::ThemedIcon::create("drive-harddisk"), size)},
      {"drive-optical",
       IconToPixbuf(Gio::ThemedIcon::create("drive-optical"), size)},
      {"drive-removable-media",
       IconToPixbuf(Gio::ThemedIcon::create("drive-removable-media"), size)},
      {"network-server",
       IconToPixbuf(Gio::ThemedIcon::create("network-server"), size)},
      {"media-floppy",
       IconToPixbuf(Gio::ThemedIcon::create("media-floppy"), size)},
  };
}

std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
CreateIconPixbufsForCommonFileTypes(int size) {
  std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>> icon_pixbufs{
      {"text/plain",
       IconToPixbuf(Gio::ThemedIcon::create("text-x-generic"), size)},
      {"application/pdf",
       IconToPixbuf(Gio::ThemedIcon::create("application-pdf"), size)},
      {"application/zip",
       IconToPixbuf(Gio::ThemedIcon::create("application-zip"), size)},
      {"application/x-tar",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-tar"), size)},
      {"application/x-rar",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-rar"), size)},
      {"application/x-7z-compressed",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-7z-compressed"),
                    size)},
      {"application/x-gzip",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-gzip"), size)},
      {"application/x-bzip",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-bzip"), size)},
      {"application/x-bzip2",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-bzip2"), size)},
      {"application/x-xz",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-xz"), size)},
      {"application/x-lzma",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-lzma"), size)},
      {"application/x-lzip",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-lzip"), size)},
      {"application/x-lrzip",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-lrzip"), size)},
      {"application/x-lrzip-compressed-tar",
       IconToPixbuf(
           Gio::ThemedIcon::create("application-x-lrzip-compressed-tar"),
           size)},
      {"application/x-lrzip-tar",
       IconToPixbuf(Gio::ThemedIcon::create("application-x-lrzip-tar"), size)},
      {"application/x-lrzip-tar-compressed",
       IconToPixbuf(
           Gio::ThemedIcon::create("application-x-lrzip-tar-compressed"),
           size)},
      {"application/x-lrzip-tar-compressed-tar",
       IconToPixbuf(
           Gio::ThemedIcon::create("application-x-lrzip-tar-compressed-tar"),
           size)},
  };
  auto directory_icons = CreateIconPixbufsForCommonDirectoryTypes(size);
  icon_pixbufs.insert(std::make_move_iterator(directory_icons.begin()),
                      std::make_move_iterator(directory_icons.end()));
  return icon_pixbufs;
}

Glib::RefPtr<Gdk::Pixbuf> LookupIconPixbufForFile(
    const std::string &path,
    const std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
        &icon_pixbufs) {
  const auto mime_type = GetFileType(path);
  if (const auto it = icon_pixbufs.find(mime_type); it != icon_pixbufs.end()) {
    return it->second;
  }
  static const auto kDefaultIcon =
      Gtk::IconTheme::get_default()->load_icon("text-x-generic", 24);
  return kDefaultIcon;
}
} // namespace art
