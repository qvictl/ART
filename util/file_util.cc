#include "util/file_util.h"

#include <algorithm>
#include <filesystem>
#include <functional>

#include <glibmm/exception.h>

#include "util/mmap.h"
#include "util/str_util.h"

namespace art {

StatusOr<int> GetNumLines(const std::string &filename, bool skip_empty) {
  try {
    MemoryMappedFile file(filename);
    const char *data = static_cast<const char *>(file.data());
    std::string_view str(data, file.size());
    const std::vector<std::string_view> lines = StrSplit(str, "\n", skip_empty);
    return lines.size();
  } catch (const std::exception &e) {
    return InternalError(e.what());
  }
}

StatusOr<std::vector<std::string>> GetLines(const std::string &filename,
                                            bool skip_empty) {
  try {
    MemoryMappedFile file(filename);
    const char *data = static_cast<const char *>(file.data());
    std::string_view str(data, file.size());
    const std::vector<std::string_view> lines = StrSplit(str, "\n", skip_empty);
    std::vector<std::string> result;
    result.reserve(lines.size());
    for (const auto &line : lines) {
      result.push_back(std::string(line));
    }
    return result;
  } catch (const std::exception &e) {
    return InternalError(e.what());
  }
}

StatusOr<std::vector<Glib::ustring>>
ListDirectory(const Glib::RefPtr<Gio::File> &dir, bool dir_only,
              bool add_hidden, Gtk::SortType sort_type) {
  std::vector<Glib::ustring> entries;

  try {
    // CD-ROM with no disc inserted are reported, but do not exist.
    if (!Glib::file_test(dir->get_path(), Glib::FILE_TEST_EXISTS)) {
      return NotFoundError(dir->get_path());
    }

    // Request the necessary attributes
    auto enumerator = dir->enumerate_children(
        "standard::name,standard::type,standard::is-hidden");

    while (true) {
      try {
        auto file = enumerator->next_file();
        if (!file) {
          break;
        }

        // Check if the file has the required attributes
        if (!file->has_attribute("standard::type")) {
          continue; // Skip files without the standard::type attribute
        }

        if (dir_only && file->get_file_type() != Gio::FILE_TYPE_DIRECTORY) {
          continue;
        }
        if (!add_hidden && file->is_hidden()) {
          continue;
        }
        entries.push_back(file->get_name());
      } catch (const Glib::Exception &exception) {
        return InternalError(exception.what());
      }
    }

  } catch (const Glib::Exception &exception) {
    return InternalError(StrCat("Failed to list subdirectories of \"",
                                dir->get_parse_name(),
                                "\": ", exception.what()));
  }

  // Sort the entries
  std::sort(entries.begin(), entries.end(),
            [sort_type](const auto &a, const auto &b) {
              if (sort_type == Gtk::SORT_ASCENDING) {
                return a < b;
              } else {
                return a > b;
              }
            });
  return entries;
}

StatusOr<std::vector<Glib::ustring>> ListDirectory(const std::string &dir,
                                                   bool dir_only,
                                                   bool add_hidden,
                                                   Gtk::SortType sort_type) {
  return ListDirectory(Gio::File::create_for_path(dir), dir_only, add_hidden,
                       sort_type);
}

bool IsRootPath(const Glib::ustring &path) {
  auto file = Gio::File::create_for_path(path);

  try {
    auto mount = file->find_enclosing_mount();
    if (mount) {
      auto root = mount->get_root();
      return root->get_path() == file->get_path();
    }
  } catch (const Glib::Error &e) {
    return false;
  }
  return false;
}

bool IsDirectory(const Glib::ustring &path) {
  return Glib::file_test(path, Glib::FILE_TEST_IS_DIR);
}

std::string GetFileType(const Glib::ustring &path) {
  return GetFileType(Gio::File::create_for_path(path));
}

std::string GetFileType(const Glib::RefPtr<Gio::File> &file) {
  try {
    auto info = file->query_info("standard::type,standard::content-type");
    return info->get_content_type();
  } catch (const Glib::Error &e) {
    return "";
  }
}

std::vector<Glib::ustring> DecomposePath(const Glib::ustring &path) {
  std::filesystem::path fs_path(path.c_str());
  std::vector<Glib::ustring> result;
  for (const auto &part : fs_path) {
#ifdef WIN32
    // On Windows, the path like "C:\Program Files\Microsoft" will be decomposed
    // into "C:", "\", "Program Files", "Microsoft". We need to merge "C:" and
    // "\" into "C:\".
    if ((part.string() == "\\" || part.string() == "/") && !result.empty() &&
        EndsWith(result.back(), ":")) {
      result.back().append(part.string());
      continue;
    }
#endif
    result.push_back(part.string());
  }
  return result;
}
} // namespace art