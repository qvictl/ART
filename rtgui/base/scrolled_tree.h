#ifndef ART_RTGUI_BASE_SCROLLED_TREE_H_
#define ART_RTGUI_BASE_SCROLLED_TREE_H_

#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>
#include <memory>

#include "rtgui/base/tree_controller.h"

namespace art {
class ScrolledTree : public Gtk::ScrolledWindow {
public:
  explicit ScrolledTree(std::shared_ptr<TreeController> tree_controller);
  virtual ~ScrolledTree() = default;
  // Allow the tree model to be changed after construction (e.g. change
  // settings)
  Gtk::TreeView *GetTreeView() { return tree_view_; }

private:
  Gtk::TreeView *tree_view_ = nullptr;
  Gtk::TreeViewColumn tree_view_column_;
  std::shared_ptr<TreeController> tree_controller_;
};
} // namespace art

#endif // ART_RTGUI_BASE_SCROLLED_TREE_H_
