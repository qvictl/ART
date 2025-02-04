#include "rtgui/base/session_tree_controller.h"

#include <array>

#include "util/file_util.h"
#include "util/logging.h"
#include <giomm.h>
#include <glibmm.h>
#include <gtkmm.h>

namespace art {
namespace {

constexpr char kSessionFileExtension[] = ".arp";

bool IsSessionName(const Glib::ustring &entry) {
  return EndsWith(entry, kSessionFileExtension);
}

bool IsSessionFile(const Glib::ustring &full_path) {
  return IsSessionName(full_path) &&
         Glib::file_test(full_path, Glib::FILE_TEST_IS_REGULAR);
}
} // namespace

SessionTreeController::SessionTreeController(
    const Glib::ustring &session_root_path)
    : FileTreeController(false), session_root_path_(session_root_path) {
  // Unlikely: If the path exists and is actually a file, return.
  if (Glib::file_test(session_root_path, Glib::FILE_TEST_IS_REGULAR)) {
    g_critical("The session directory path already taken by a file: %s",
               session_root_path.c_str());
    return;
  } else if (!Glib::file_test(session_root_path, Glib::FILE_TEST_EXISTS)) {
    Glib::RefPtr<Gio::File> file =
        Gio::File::create_for_path(session_root_path);
    try {
      file->make_directory_with_parents();
    } catch (const Glib::Error &e) {
      g_critical("Failed to create the session directory: %s",
                 session_root_path.c_str());
      return;
    }
  } else {
    auto sub_entries = ListDirectory(session_root_path, /*dir_only=*/false,
                                     /*add_hidden=*/true, Gtk::SORT_ASCENDING);
    if (!sub_entries.ok()) {
      g_critical("Failed to list the session directory: %s",
                 session_root_path.c_str());
      return;
    }
    for (const auto &entry : sub_entries.value()) {
      const auto path = BuildPath(entry);
      if (IsDirectory(path) || IsSessionName(entry)) {
        AddRootEntry(entry);
        NotifyEntryChanged(LookUpSubEntry({}, std::array{entry}));
      }
    }
  }
}

Glib::ustring SessionTreeController::GetDisplayName(
    const Gtk::TreeModel::const_iterator &iter) const {
  const auto full_path = BuildPath(iter->get_value(columns_.full_path));
  auto key_name = GetKeyName(iter);
  if (IsDirectory(full_path)) {
    return key_name;
  } else {
    // remove the extension
    key_name.erase(key_name.size() - sizeof(kSessionFileExtension) + 1);
    const auto num_photos_iter = full_path_to_num_photos_map_.find(full_path);
    const int num_photos = num_photos_iter != full_path_to_num_photos_map_.end()
                               ? num_photos_iter->second
                               : 0; // should no happen
    return Glib::ustring::compose("%1 (%2)", key_name, num_photos);
  }
}

std::vector<Glib::ustring> SessionTreeController::GetSubEntries(
    const Gtk::TreeModel::const_iterator &iter) const {
  const auto full_path = BuildPath(iter->get_value(columns_.full_path));
  auto entries = ListDirectory(full_path, /*dir_only=*/false,
                               /*add_hidden=*/true, Gtk::SORT_ASCENDING);
  if (!entries.ok()) {
    return {};
  }
  const auto it = std::remove_if(
      entries->begin(), entries->end(), [this, &iter](const auto &entry) {
        return !IsDirectory(BuildPath(GetKeyName(iter), entry)) &&
               !IsSessionName(entry);
      });
  entries->erase(it, entries->end());
  return *entries;
}

Glib::RefPtr<Gdk::Pixbuf> SessionTreeController::GetDefaultIconPixbuf(
    const Gtk::TreeModel::const_iterator &iter) {
  // TODO(zoulu): Use a different icon for session files.
  return open_folder_pix_;
}

Glib::RefPtr<Gdk::Pixbuf> SessionTreeController::GetOpenedIconPixbuf(
    const Gtk::TreeModel::const_iterator &iter) {
  return open_folder_pix_;
}

Glib::ustring
SessionTreeController::BuildPath(const Glib::ustring &entry) const {
  return Glib::build_filename(session_root_path_, entry);
}
Glib::ustring
SessionTreeController::BuildPath(const Glib::ustring &dir,
                                 const Glib::ustring &entry) const {
  return Glib::build_filename(session_root_path_, dir, entry);
}

void SessionTreeController::NotifyEntryChanged(
    const Gtk::TreeModel::iterator &iter) {
  FileTreeController::NotifyEntryChanged(iter);

  // Update the number of photos in the session file.
  const auto filename = iter->get_value(columns_.full_path);
  const auto full_filename = BuildPath(filename);
  if (IsSessionFile(full_filename)) {
    UpdateNumPhotos(full_filename);
  } else {
    const auto sub_entries = GetSubEntries(iter);
    for (const auto &entry : sub_entries) {
      const auto full_path = BuildPath(filename, entry);
      if (IsSessionFile(full_path)) {
        UpdateNumPhotos(full_path);
      }
    }
  }
  Info(DebugString());
}

void SessionTreeController::UpdateNumPhotos(
    const Glib::ustring &full_filename) {
  full_path_to_num_photos_map_[full_filename] =
      GetNumLines(full_filename, /*skip_empty=*/true);
}

std::string SessionTreeController::DebugString() const {
  return StrFormat(
      "{%s}", StrJoin(full_path_to_num_photos_map_, "},{",
                      [](std::string *out, const auto &pair) {
                        StrAppend(out, StrCat(pair.first, ":", pair.second));
                      }));
}

Glib::ustring SessionTreeController::GetFullPath(
    const Gtk::TreeModel::const_iterator &iter) const {
  return BuildPath(iter->get_value(columns_.full_path));
}
} // namespace art
