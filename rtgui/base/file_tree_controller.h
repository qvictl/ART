#ifndef ART_RTGUI_BASE_FILE_TREE_CONTROLLER_H_
#define ART_RTGUI_BASE_FILE_TREE_CONTROLLER_H_
#include "rtgui/base/tree_controller.h"

#include <unordered_map>

#include <gdkmm/pixbuf.h>
#include <giomm/filemonitor.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <gtkmm/treemodelcolumn.h>

namespace art {

class FileTreeController : public TreeController {
public:
  explicit FileTreeController(bool dir_only);
  virtual ~FileTreeController() = default;

  // Virtual functions can be further overridden by subclasses
  virtual Glib::ustring
  GetDisplayName(const Gtk::TreeModel::const_iterator &iter) const override;

  // Add filesystem related virtual functions
  virtual Glib::RefPtr<Gdk::Pixbuf>
  GetDefaultIconPixbuf(const Gtk::TreeModel::const_iterator &iter);
  virtual Glib::RefPtr<Gdk::Pixbuf>
  GetOpenedIconPixbuf(const Gtk::TreeModel::const_iterator &iter);

  virtual void
  NotifyEntryChanged(const Gtk::TreeModel::iterator &iter) override;

  using SelectionSignal =
      sigc::signal<void, const Glib::ustring &, const Glib::ustring &>;
  SelectionSignal &GetSelectionSignal() { return selection_signal_; }

  void SetupTreeViewColumn(const Glib::RefPtr<Gtk::TreeSelection> &selection,
                           Gtk::TreeViewColumn *column) const override;

  void
  RenderRow(Gtk::CellRenderer *renderer,
            const Gtk::TreeModel::const_iterator &iter,
            const Glib::RefPtr<Gtk::TreeSelection> &selection) const override;

  Glib::ustring
  GetKeyName(const Gtk::TreeModel::const_iterator &iter) const override;

  std::vector<Glib::ustring>
  GetSubEntries(const Gtk::TreeModel::const_iterator &iter) const override;

  void AddSubEntry(const Gtk::TreeModel::iterator &iter,
                   const Glib::ustring &entry, bool include_sub_child) override;

  void RowActivated(const Gtk::TreeModel::Path &path,
                    Gtk::TreeViewColumn *column) const override;

  std::vector<Glib::ustring>
  DecomposePath(const Glib::ustring &path) const override;

  virtual Glib::ustring
  GetFullPath(const Gtk::TreeModel::const_iterator &iter) const;

protected:
  const bool dir_only_ = false;
  SelectionSignal selection_signal_;
  struct DirTreeColumns : public Gtk::TreeModelColumnRecord {
    Gtk::TreeModelColumn<Glib::ustring> filename;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon_default;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> icon_opened;
    Gtk::TreeModelColumn<Glib::ustring> full_path;
    Gtk::TreeModelColumn<Glib::RefPtr<Gio::FileMonitor>> monitor;

    DirTreeColumns() {
      add(icon_default);
      add(icon_opened);
      add(filename);
      add(full_path);
      add(monitor);
    }
  } columns_;

  // <file mime type, icon pixbuf>
  std::unordered_map<std::string, Glib::RefPtr<Gdk::Pixbuf>>
      file_type_to_icon_pix_map_;
  Glib::RefPtr<Gdk::Pixbuf> open_folder_pix_;
};

} // namespace art

#endif // ART_RTGUI_BASE_FILE_TREE_CONTROLLER_H_
