#include "rtgui/base/scrolled_tree.h"
#include "rtgui/base/session_tree_controller.h"
#include <gtkmm.h>

#include <iostream>

#include <algorithm>
#include <filesystem>
#include <glibmm/refptr.h>
#include <memory>

#include "util/logging.h"

class MyWindow : public Gtk::Window {
public:
  MyWindow() {
    // Set up the window
    set_title("Gtkmm TreeView Example");
    set_default_size(400, 200);

    // Set up the TreeView
    auto controller = std::make_shared<art::SessionTreeController>("C:\\test");
    auto *scrolled_tree = Gtk::make_managed<art::ScrolledTree>(controller);
    scrolled_tree->GetTreeView()->signal_row_activated().connect(
        [&controller](const Gtk::TreeModel::Path &path,
                      Gtk::TreeViewColumn *column) {
          art::Info("Selecting...");
          art::Info(
              "Selected: %s",
              controller->GetFullPath(controller->GetModel()->get_iter(path)));
        });
    auto *vbox = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL);
    vbox->pack_start(*scrolled_tree, true, true, 0);

    // Add TreeView to the window
    add(*vbox);
    show_all();
  }
};

int main(int argc, char *argv[]) {
  Glib::init();
  Gio::init();
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");
  MyWindow window;
  return app->run(window);
}

// #include <gtkmm.h>

// class ModelColumns : public Gtk::TreeModel::ColumnRecord {
// public:
//   ModelColumns() {
//     add(m_col_name);
//     add(m_col_value);
//   }

//   Gtk::TreeModelColumn<Glib::ustring> m_col_name; // Column for the name
//   Gtk::TreeModelColumn<int> m_col_value;          // Column for the value
// };

// int main(int argc, char *argv[]) {
//   Gtk::Main app(argc, argv);

//   ModelColumns columns;
//   Glib::RefPtr<Gtk::TreeStore> tree_store = Gtk::TreeStore::create(columns);
//   Gtk::TreeView tree_view;
//   tree_view.set_model(tree_store);

//   tree_view.append_column("Name", columns.m_col_name);
//   tree_view.append_column("Value", columns.m_col_value);

//   Gtk::TreeModel::Row row = *(tree_store->append());
//   row[columns.m_col_name] = "Parent 1";
//   row[columns.m_col_value] = 100;

//   Gtk::TreeModel::Row child_row = *(tree_store->append(row.children()));
//   child_row[columns.m_col_name] = "Child 1";
//   child_row[columns.m_col_value] = 50;

//   Gtk::TreeModel::Row row2 = *(tree_store->append());
//   row2[columns.m_col_name] = "Parent 2";
//   row2[columns.m_col_value] = 200;

//   Gtk::ScrolledWindow scrolled_window;
//   scrolled_window.add(tree_view);
//   scrolled_window.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

//   Gtk::Window window;
//   window.set_default_size(400, 300);
//   window.add(scrolled_window);
//   window.show_all();

//   Gtk::Main::run(window);

//   return 0;
// }