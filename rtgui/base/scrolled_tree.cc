#include "rtgui/base/scrolled_tree.h"

#include <algorithm>
#include <glibmm/refptr.h>

namespace art {
namespace {
class ModelColumns : public Gtk::TreeModel::ColumnRecord {
public:
  ModelColumns() {
    add(m_col_name);
    add(m_col_value);
  }

  Gtk::TreeModelColumn<Glib::ustring> m_col_name; // Column for the name
  Gtk::TreeModelColumn<int> m_col_value;          // Column for the value
};
} // namespace

ScrolledTree::ScrolledTree(std::shared_ptr<TreeController> tree_controller)
    : tree_controller_(std::move(tree_controller)) {
  set_can_focus(true);
  set_shadow_type(Gtk::SHADOW_NONE);
  set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  property_window_placement().set_value(Gtk::CORNER_TOP_LEFT);

  tree_view_ = Gtk::make_managed<Gtk::TreeView>();
  tree_view_->set_headers_visible();
  tree_view_->set_headers_clickable();
  tree_view_->set_rules_hint(false);
  tree_view_->set_reorderable(false);
  tree_view_->set_enable_search(false);

  tree_view_->set_model(tree_controller_->GetModel());
  tree_controller_->SetupTreeViewColumn(tree_view_->get_selection(),
                                        &tree_view_column_);
  tree_view_->append_column(tree_view_column_);
  // tree_controller->AddRootEntry("C:");
  tree_view_->signal_row_expanded().connect(
      [this](const Gtk::TreeModel::iterator &iter,
             const Gtk::TreeModel::Path & /*path*/) {
        tree_controller_->UpdateChildren(iter, /*include_sub_child=*/true);
        tree_view_->queue_draw();
      });
  tree_view_->signal_row_activated().connect(
      [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        tree_controller_->RowActivated(path, column);
        tree_view_->queue_draw();
      });

  add(*tree_view_);
}

} // namespace art