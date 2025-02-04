#include "rtgui/base/tree_controller.h"

#include "util/logging.h"
#include "util/str_util.h"
#include <algorithm>
#include <set>

namespace art {

void TreeController::UpdateChildren(const Gtk::TreeModel::iterator &iter,
                                    bool include_sub_child) {
  const auto &[added, remained, removed] = DetectChanges(iter);
  if (added.empty() && removed.empty() &&
      (remained.empty() || !include_sub_child)) {
    return;
  }
  // We will disable model's sorting because it decreases speed of inserting new
  // items in list tree dramatically. Therefore will do:
  // 1) Disable sorting in model
  // 2) Manually sort data in the order determined by the options
  // 3) Enable sorting in model again for UI (sorting by click on header)

  int prev_sort_column;
  Gtk::SortType prev_sort_type;
  tree_store_->get_sort_column_id(prev_sort_column, prev_sort_type);
  tree_store_->set_sort_column(Gtk::TreeSortable::DEFAULT_UNSORTED_COLUMN_ID,
                               Gtk::SORT_ASCENDING);

  std::vector<Gtk::TreeModel::iterator> to_remove;
  std::vector<Gtk::TreeModel::iterator> to_update;
  to_remove.reserve(removed.size());
  to_update.reserve(remained.size());
  for (const auto &child : iter->children()) {
    const auto name = GetKeyName(child);
    if (remained.find(name) != remained.end()) {
      to_update.push_back(child);
    } else if (removed.find(name) != removed.end()) {
      to_remove.push_back(child);
    }
  }
  std::for_each(to_remove.begin(), to_remove.end(),
                [this](const auto &child) { tree_store_->erase(child); });
  std::for_each(to_update.begin(), to_update.end(),
                [this](const auto &child) { UpdateEntry(child); });

  for (const auto &entry : added) {
    AddSubEntry(iter, entry, include_sub_child);
  }

  tree_store_->set_sort_column(prev_sort_column, prev_sort_type);
  NotifyEntryChanged(iter);
}

void TreeController::AddRootEntry(const Glib::ustring &entry) {
  AddSubEntry(Gtk::TreeModel::iterator(), entry, /*include_sub_child=*/true);
}

void TreeController::UpdateEntry(const Gtk::TreeModel::iterator &iter) {
  UpdateChildren(iter, /*include_sub_child=*/false);
}

void TreeController::SetModel(Glib::RefPtr<Gtk::TreeStore> tree_store) {
  tree_store_ = tree_store;
}

std::tuple<std::set<Glib::ustring>, std::set<Glib::ustring>,
           std::set<Glib::ustring>>
TreeController::DetectChanges(
    const Gtk::TreeModel::const_iterator &iter) const {

  auto new_entries = GetSubEntries(iter);
  std::vector<Glib::ustring> old_entries;
  old_entries.reserve(iter->children().size());
  for (const auto &child : iter->children()) {
    old_entries.push_back(GetKeyName(child));
  }

  std::sort(old_entries.begin(), old_entries.end());

  std::vector<Glib::ustring> added;
  std::vector<Glib::ustring> remained;
  std::vector<Glib::ustring> removed;
  std::set_difference(new_entries.begin(), new_entries.end(),
                      old_entries.begin(), old_entries.end(),
                      std::back_inserter(added));
  std::set_difference(old_entries.begin(), old_entries.end(),
                      new_entries.begin(), new_entries.end(),
                      std::back_inserter(removed));
  std::set_intersection(new_entries.begin(), new_entries.end(),
                        old_entries.begin(), old_entries.end(),
                        std::back_inserter(remained));

  return {{added.begin(), added.end()},
          {remained.begin(), remained.end()},
          {removed.begin(), removed.end()}};
}

Gtk::TreeModel::const_iterator
TreeController::LookUpSubEntry(const Gtk::TreeModel::const_iterator &iter,
                               const Span<Glib::ustring> &sub_path) const {
  if (sub_path.empty()) {
    return iter;
  }
  const auto &children = (iter) ? iter->children() : tree_store_->children();
  for (const auto &child : children) {
    if (GetKeyName(child) == sub_path.front()) {
      if (sub_path.size() == 1) {
        return child;
      } else {
        return LookUpSubEntry(child, sub_path.subspan(1));
      }
    }
  }
  // Not found
  return {};
}
} // namespace art
