#include <filesystem>
#include <gtkmm.h>
#include <iostream>

namespace fs = std::filesystem;

class FileBrowser : public Gtk::Window {
public:
  FileBrowser() {
    // Window setup
    set_title("Filesystem Tree Browser");
    set_default_size(400, 600);

    // Main vertical box
    Gtk::Box *vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 5));
    add(*vbox);

    // Controls (filter and search)
    Gtk::Box *controls_box =
        Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    vbox->pack_start(*controls_box, Gtk::PACK_SHRINK);

    // Directory-only toggle
    directory_only_toggle.set_label("Directory Only");
    controls_box->pack_start(directory_only_toggle, Gtk::PACK_SHRINK);
    directory_only_toggle.signal_toggled().connect(
        sigc::mem_fun(*this, &FileBrowser::on_filter_changed));

    // Suffix filter entry
    suffix_entry.set_placeholder_text("Filter by suffix (e.g., .txt)");
    controls_box->pack_start(suffix_entry, Gtk::PACK_EXPAND_WIDGET);
    suffix_entry.signal_changed().connect(
        sigc::mem_fun(*this, &FileBrowser::on_filter_changed));

    // Custom display string entry
    custom_display_entry.set_placeholder_text(
        "Custom display format (e.g., {filename})");
    controls_box->pack_start(custom_display_entry, Gtk::PACK_EXPAND_WIDGET);
    custom_display_entry.signal_changed().connect(
        sigc::mem_fun(*this, &FileBrowser::on_filter_changed));

    // TreeView setup
    tree_store = Gtk::TreeStore::create(columns);
    tree_view.set_model(tree_store);
    tree_view.append_column("Name", columns.name);
    tree_view.append_column("Path", columns.path);
    vbox->pack_start(tree_view, Gtk::PACK_EXPAND_WIDGET);

    // Populate the tree with the root directory
    populate_tree("/");

    show_all_children();
  }

protected:
  // Tree model columns
  class ModelColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ModelColumns() {
      add(name);
      add(path);
    }
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> path;
  };

  ModelColumns columns;

  // Widgets
  Gtk::TreeView tree_view;
  Glib::RefPtr<Gtk::TreeStore> tree_store;
  Gtk::ToggleButton directory_only_toggle;
  Gtk::Entry suffix_entry;
  Gtk::Entry custom_display_entry;

  // Populate the tree with files and directories
  void populate_tree(const std::string &root_path) {
    tree_store->clear();
    try {
      for (const auto &entry : fs::directory_iterator(root_path)) {
        if (should_display(entry)) {
          add_row(entry);
        }
      }
    } catch (const fs::filesystem_error &e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  // Add a row to the tree
  void add_row(const fs::directory_entry &entry) {
    Gtk::TreeModel::Row row = *(tree_store->append());
    std::string display_name = get_display_name(entry);
    row[columns.name] = display_name;
    row[columns.path] = entry.path().string();
  }

  // Check if an entry should be displayed based on filters
  bool should_display(const fs::directory_entry &entry) {
    bool is_directory = entry.is_directory();
    bool matches_suffix = true;

    // Directory-only filter
    if (directory_only_toggle.get_active() && !is_directory) {
      return false;
    }

    // Suffix filter
    std::string suffix = suffix_entry.get_text();
    if (!suffix.empty() && !is_directory) {
      std::string path = entry.path().string();
      if (path.size() < suffix.size() ||
          path.compare(path.size() - suffix.size(), suffix.size(), suffix) !=
              0) {
        return false;
      }
    }

    return true;
  }

  // Get the display name for an entry
  std::string get_display_name(const fs::directory_entry &entry) {
    std::string custom_format = custom_display_entry.get_text();
    if (!custom_format.empty()) {
      // Replace placeholders with actual values
      std::string display_name = custom_format;
      size_t pos = display_name.find("{filename}");
      if (pos != std::string::npos) {
        display_name.replace(pos, 10, entry.path().filename().string());
      }
      pos = display_name.find("{path}");
      if (pos != std::string::npos) {
        display_name.replace(pos, 6, entry.path().string());
      }
      return display_name;
    }
    return entry.path().filename().string();
  }

  // Handle filter changes
  void on_filter_changed() { populate_tree("/"); }
};

int main(int argc, char *argv[]) {
  auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

  FileBrowser browser;
  return app->run(browser);
}