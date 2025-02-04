#ifndef ART_RTGUI_BASE_SESSION_TREE_CONTROLLER_H_
#define ART_RTGUI_BASE_SESSION_TREE_CONTROLLER_H_

#include "rtgui/base/file_tree_controller.h"

namespace art {
// Session tree is equivalent to LR's collection set. The tree can only
// have one level of subdirectories. (In theory, user can add subdirectories
// using the file browser, but we don't support that in the UI.)
// Directories act as 'Session set' and files act as 'Session'.
class SessionTreeController : public FileTreeController {
public:
  explicit SessionTreeController(const Glib::ustring &session_root_path);
  virtual ~SessionTreeController() = default;

  Glib::ustring
  GetDisplayName(const Gtk::TreeModel::const_iterator &iter) const override;

  std::vector<Glib::ustring>
  GetSubEntries(const Gtk::TreeModel::const_iterator &iter) const override;

  Glib::RefPtr<Gdk::Pixbuf>
  GetDefaultIconPixbuf(const Gtk::TreeModel::const_iterator &iter) override;
  Glib::RefPtr<Gdk::Pixbuf>
  GetOpenedIconPixbuf(const Gtk::TreeModel::const_iterator &iter) override;

  void NotifyEntryChanged(const Gtk::TreeModel::iterator &iter) override;

  Glib::ustring
  GetFullPath(const Gtk::TreeModel::const_iterator &iter) const override;

private:
  const Glib::ustring session_root_path_;
  // <file full path, number of photos in the file>
  std::map<Glib::ustring, int> full_path_to_num_photos_map_;
  void UpdateNumPhotos(const Glib::ustring &full_filename);
  Glib::ustring BuildPath(const Glib::ustring &entry) const;
  Glib::ustring BuildPath(const Glib::ustring &dir,
                          const Glib::ustring &entry) const;
  std::string DebugString() const;
};

} // namespace art

#endif // ART_RTGUI_BASE_SESSION_TREE_CONTROLLER_H_
