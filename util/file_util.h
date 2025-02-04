#ifndef ART_UTIL_FILE_UTIL_H_
#define ART_UTIL_FILE_UTIL_H_

#include <optional>
#include <string>
#include <vector>

#include "util/status.h"
#include <giomm.h>
#include <glibmm/refptr.h>
#include <gtkmm/enums.h>

namespace art {
StatusOr<int> GetNumLines(const std::string &filename, bool skip_empty);
StatusOr<std::vector<std::string>> GetLines(const std::string &filename,
                                            bool skip_empty);

StatusOr<std::vector<Glib::ustring>>
ListDirectory(const Glib::RefPtr<Gio::File> &dir, bool dir_only,
              bool add_hidden, Gtk::SortType sort_type);

StatusOr<std::vector<Glib::ustring>> ListDirectory(const std::string &dir,
                                                   bool dir_only,
                                                   bool add_hidden,
                                                   Gtk::SortType sort_type);

std::vector<Glib::ustring> DecomposePath(const Glib::ustring &path);

bool IsRootPath(const Glib::ustring &path);

bool IsDirectory(const Glib::ustring &path);

std::string GetFileType(const Glib::ustring &path);
std::string GetFileType(const Glib::RefPtr<Gio::File> &file);

} // namespace art

#endif // ART_UTIL_FILE_UTIL_H_
