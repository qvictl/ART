#include "rtgui/base/file_tree_controller.h"

#include <gtkmm/cellrendererpixbuf.h>
#include <gtkmm/cellrenderertext.h>
#include <gtkmm/icontheme.h>
#include <iostream>

#include "util/file_util.h"
#include "util/icon_util.h"
#include "util/logging.h"

namespace art {

namespace {
struct Options {
  bool fbShowHidden = false;
  Gtk::SortType dirBrowserSortType = Gtk::SortType::SORT_ASCENDING;
  struct RTSettings {
    bool verbose = false;
  } rtSettings;
} options;

} // namespace

FileTreeController::FileTreeController(bool dir_only) : dir_only_(dir_only) {
  SetModel(Gtk::TreeStore::create(columns_));
  constexpr int kIconSizeInPixel = 24;
  if (dir_only) {
    file_type_to_icon_pix_map_ =
        CreateIconPixbufsForCommonDirectoryTypes(kIconSizeInPixel);
  } else {
    file_type_to_icon_pix_map_ =
        CreateIconPixbufsForCommonFileTypes(kIconSizeInPixel);
  }
  open_folder_pix_ = IconToPixbuf(IconForOpenedDirectory(), kIconSizeInPixel);
}

void FileTreeController::SetupTreeViewColumn(
    const Glib::RefPtr<Gtk::TreeSelection> &selection,
    Gtk::TreeViewColumn *column) const {
  auto *pixbuf = Gtk::make_managed<Gtk::CellRendererPixbuf>();
  column->pack_start(*pixbuf, false);
  column->add_attribute(*pixbuf, "pixbuf-expander-closed",
                        columns_.icon_default);
  column->add_attribute(*pixbuf, "pixbuf", columns_.icon_default);
  column->add_attribute(*pixbuf, "pixbuf-expander-open", columns_.icon_opened);

  auto *text = Gtk::make_managed<Gtk::CellRendererText>();
  text->property_ypad() = 0;
  column->pack_start(*text);
  column->add_attribute(*text, "text", columns_.filename);
  column->set_cell_data_func(
      *text, sigc::bind(sigc::mem_fun(*this, &FileTreeController::RenderRow),
                        selection));
  tree_store_->signal_sort_column_changed().connect(
      [column]() { options.dirBrowserSortType = column->get_sort_order(); });

  pixbuf->property_ypad() = 0;
}

void FileTreeController::RenderRow(
    Gtk::CellRenderer *renderer, const Gtk::TreeModel::const_iterator &iter,
    const Glib::RefPtr<Gtk::TreeSelection> &selection) const {
  const Glib::ustring &display_name = GetDisplayName(iter);
  const Glib::ustring &name =
      selection->is_selected(iter)
          ? Glib::ustring::compose("<b>%1</b>", display_name)
          : display_name;

  Gtk::CellRendererText *text_renderer =
      dynamic_cast<Gtk::CellRendererText *>(renderer);
  if (text_renderer != nullptr) {
    text_renderer->property_markup() = name;
  }
}

Glib::ustring FileTreeController::GetKeyName(
    const Gtk::TreeModel::const_iterator &iter) const {
  return (*iter)[columns_.filename];
}

Glib::ustring FileTreeController::GetDisplayName(
    const Gtk::TreeModel::const_iterator &iter) const {
  // The display name is the same as the key name.
  return GetKeyName(iter);
}

std::vector<Glib::ustring> FileTreeController::GetSubEntries(
    const Gtk::TreeModel::const_iterator &iter) const {
  auto result = art::ListDirectory(iter->get_value(columns_.full_path),
                                   /*dir_only=*/dir_only_,
                                   /*add_hidden=*/options.fbShowHidden,
                                   /*sort_type=*/options.dirBrowserSortType);
  if (!result.ok()) {
    if (options.rtSettings.verbose) {
      std::cerr << Glib::ustring::compose(
          "Failed to list subdirectories of %1: %2",
          (*iter)[columns_.full_path], result.ToString());
    }
    return {};
  } else {
    return result.value();
  }
}

void FileTreeController::AddSubEntry(const Gtk::TreeModel::iterator &iter,
                                     const Glib::ustring &entry,
                                     bool include_sub_child) {
  Gtk::TreeModel::iterator child =
      iter ? tree_store_->append(iter->children()) : tree_store_->append();
  child->set_value(columns_.filename, entry);
  Glib::ustring full_path =
      iter ? Glib::ustring(Glib::build_filename(
                 iter->get_value(columns_.full_path), entry))
           : entry;
  child->set_value(columns_.full_path, full_path);
  child->set_value(columns_.icon_default, GetDefaultIconPixbuf(child));
  child->set_value(columns_.icon_opened, GetOpenedIconPixbuf(child));
  if (include_sub_child) {
    // Get content of subdirectories
    UpdateChildren(child, /*include_sub_child=*/false);
  }
  // NOTE(zoulu): I don't know what this is for.
  // tree_store_->append(child->children())
  //     ->set_value(columns_.filename, Glib::ustring("foo"));
}

void FileTreeController::NotifyEntryChanged(
    const Gtk::TreeModel::iterator &iter) {
  if (iter && !iter->get_value(columns_.monitor)) {
    auto dir = Gio::File::create_for_path(iter->get_value(columns_.full_path));
    auto monitor = dir->monitor_directory();
    iter->set_value(columns_.monitor, monitor);
    monitor->signal_changed().connect(
        [this, iter](const Glib::RefPtr<Gio::File> &,
                     const Glib::RefPtr<Gio::File> &, Gio::FileMonitorEvent) {
          UpdateChildren(iter, /*include_sub_child=*/true);
        });
  }
  tree_store_->row_changed(tree_store_->get_path(iter), iter);
}

void FileTreeController::RowActivated(const Gtk::TreeModel::Path &path,
                                      Gtk::TreeViewColumn *column) const {
  Glib::ustring full_path =
      tree_store_->get_iter(path)->get_value(columns_.full_path);

  if (Glib::file_test(full_path, Glib::FILE_TEST_IS_DIR)) {
    selection_signal_(full_path, Glib::ustring());
  }
}

Glib::RefPtr<Gdk::Pixbuf> FileTreeController::GetDefaultIconPixbuf(
    const Gtk::TreeModel::const_iterator &iter) {
  return LookupIconPixbufForFile(iter->get_value(columns_.full_path),
                                 file_type_to_icon_pix_map_);
}

Glib::RefPtr<Gdk::Pixbuf> FileTreeController::GetOpenedIconPixbuf(
    const Gtk::TreeModel::const_iterator &iter) {
  return open_folder_pix_;
}

std::vector<Glib::ustring>
FileTreeController::DecomposePath(const Glib::ustring &path) const {
  return ::art::DecomposePath(path);
}

Glib::ustring FileTreeController::GetFullPath(
    const Gtk::TreeModel::const_iterator &iter) const {
  return iter->get_value(columns_.full_path);
}
} // namespace art
