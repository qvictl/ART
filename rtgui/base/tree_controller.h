#ifndef ART_RTGUI_BASE_TREE_CONTROLLER_H_
#define ART_RTGUI_BASE_TREE_CONTROLLER_H_

#include <set>
#include <tuple>

#include <glibmm/refptr.h>
#include <gtkmm/cellrenderer.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treeselection.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeviewcolumn.h>

#include "util/span.h"

namespace art {
class TreeController {
public:
  TreeController() = default;
  virtual ~TreeController() = default;

  // Pure virtual functions
  virtual void
  SetupTreeViewColumn(const Glib::RefPtr<Gtk::TreeSelection> &selection,
                      Gtk::TreeViewColumn *column) const = 0;

  // Get called when the tree view needs to render a row. e.g. when the row is
  // first displayed or when the row recieves mouse hover and click event.
  virtual void
  RenderRow(Gtk::CellRenderer *renderer,
            const Gtk::TreeModel::const_iterator &iter,
            const Glib::RefPtr<Gtk::TreeSelection> &selection) const = 0;

  // Key name is used to identify the entry in the tree model.
  virtual Glib::ustring
  GetKeyName(const Gtk::TreeModel::const_iterator &iter) const = 0;

  // Display name is used to show the entry in the tree view. It can be
  // different from the key name.
  // NOTE(zoulu): This function gets called by RenderRow(), so it should be
  // fast (don't do too much file operation, etc.).
  virtual Glib::ustring
  GetDisplayName(const Gtk::TreeModel::const_iterator &iter) const = 0;

  virtual std::vector<Glib::ustring>
  GetSubEntries(const Gtk::TreeModel::const_iterator &iter) const = 0;

  virtual void AddSubEntry(const Gtk::TreeModel::iterator &iter,
                           const Glib::ustring &entry,
                           bool include_sub_child) = 0;

  virtual void RowActivated(const Gtk::TreeModel::Path &path,
                            Gtk::TreeViewColumn *column) const = 0;

  // Decompose a full tree path into a vector of strings. The first element is
  // the root entry, and the last element is the leaf entry.
  virtual std::vector<Glib::ustring>
  DecomposePath(const Glib::ustring &path) const = 0;

  // Virtual functions
  virtual void NotifyEntryChanged(const Gtk::TreeModel::iterator &iter) {}

  // Non-virtual functions
  void UpdateChildren(const Gtk::TreeModel::iterator &iter,
                      bool include_sub_child);

  void AddRootEntry(const Glib::ustring &entry);

  void UpdateEntry(const Gtk::TreeModel::iterator &iter);

  void SetModel(Glib::RefPtr<Gtk::TreeStore> tree_store);
  Glib::RefPtr<Gtk::TreeStore> &GetModel() { return tree_store_; }

  // Pass invalid iterator to search in the root level.
  Gtk::TreeModel::const_iterator
  LookUpSubEntry(const Gtk::TreeModel::const_iterator &iter,
                 const Span<Glib::ustring> &sub_path) const;

protected:
  Glib::RefPtr<Gtk::TreeStore> tree_store_;
  // <Added, Remained, Removed>
  std::tuple<std::set<Glib::ustring>, std::set<Glib::ustring>,
             std::set<Glib::ustring>>
  DetectChanges(const Gtk::TreeModel::const_iterator &iter) const;
};
} // namespace art

#endif // ART_RTGUI_BASE_TREE_CONTROLLER_H_