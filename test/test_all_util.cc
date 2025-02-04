#include <cstdarg>
#include <cstdio>
#include <giomm.h>
#include <glib.h>
#include <string>

#include "util/file_util.h"
#include "util/keyboard_util.h"
#include "util/logging.h"
#include "util/mmap.h"
#include "util/span.h"
#include "util/status.h"
#include "util/str_util.h"

namespace {
constexpr char kFileName[] = "___test_file___.txt";
constexpr char kContent[] = "Hello,\nWorld!\n\n";
// ScopedTestFile creates a test file in the current directory and deletes it
// when it goes out of scope (RAII). e.g. test finishes or exception is thrown.
struct ScopedTestFile {
  ScopedTestFile() { Glib::file_set_contents(kFileName, kContent); }
  ~ScopedTestFile() { std::remove(kFileName); }
};
} // namespace

// Unit Test using GLib Testing Framework
static void test_str_format() {
  const auto foo = art::StrFormat("foo");
  g_assert_cmpstr(foo.c_str(), ==, "foo");
  const auto hello = art::StrFormat("Hello, %s!", "World");
  g_assert_cmpstr(hello.c_str(), ==, "Hello, World!");
  // Supports format std::string and Glib::ustring with %s
  const std::string world = "World";
  const auto hello2 = art::StrFormat("Hello, %s!", world);
  g_assert_cmpstr(hello2.c_str(), ==, "Hello, World!");
  const Glib::ustring world_u = "World";
  const auto hello3 = art::StrFormat("Hello, %s!", world_u);
  g_assert_cmpstr(hello3.c_str(), ==, "Hello, World!");

  const auto int_format = art::StrFormat("%d + %d = %d", 2, 3, 5);
  g_assert_cmpstr(int_format.c_str(), ==, "2 + 3 = 5");
  const auto float_format = art::StrFormat("%f", 3.14159);
  g_assert_cmpstr(float_format.c_str(), ==, "3.141590");
}

static void test_str_join() {
  const std::vector<std::string> strs = {"a", "b", "c"};
  const auto joined = art::StrJoin(strs, ",");
  g_assert_cmpstr(joined.c_str(), ==, "a,b,c");

  const std::vector<std::string> empty;
  const auto empty_joined = art::StrJoin(empty, ",");
  g_assert_cmpstr(empty_joined.c_str(), ==, "");

  const std::vector<std::string> single = {"a"};
  const auto single_joined = art::StrJoin(single, ",");
  g_assert_cmpstr(single_joined.c_str(), ==, "a");

  const std::vector<double> doubles = {1.1, 2.2, 3.3};
  const auto double_joined = art::StrJoin(doubles, ",");
  g_assert_cmpstr(double_joined.c_str(), ==, "1.1,2.2,3.3");

  // Format anything that's not directly convertible to string
  std::map<int, std::string> map = {{1, "a"}, {2, "b"}, {3, "c"}};
  const auto map_joined = art::StrFormat(
      "{%s}",
      art::StrJoin(
          map, "},{",
          [](std::string *out, const std::pair<int, std::string> &pair) {
            art::StrAppend(out, art::StrCat(pair.first, ":", pair.second));
          }));
  g_assert_cmpstr(map_joined.c_str(), ==, "{1:a},{2:b},{3:c}");
}

static void test_str_cat() {
  const auto hello = art::StrCat("Hello, ", "World", "!");
  g_assert_cmpstr(hello.c_str(), ==, "Hello, World!");

  const auto int_format = art::StrCat(2, " + ", 3, " = ", 5);
  g_assert_cmpstr(int_format.c_str(), ==, "2 + 3 = 5");

  const auto float_format = art::StrCat(3.14159);
  // the first char is 3
  g_assert_cmpint(float_format[0], ==, '3');
}

static void test_status() {
  const auto ok = art::OkStatus();
  g_assert_cmpint(static_cast<int>(ok.code()), ==,
                  static_cast<int>(art::StatusCode::kOk));
  g_assert_cmpstr(ok.message().data(), ==, "");

  const auto invalid = art::InvalidArgumentError("Invalid argument");
  g_assert_cmpint(static_cast<int>(invalid.code()), ==,
                  static_cast<int>(art::StatusCode::kInvalidArgument));
  g_assert_cmpstr(invalid.message().data(), ==, "Invalid argument");
}

art::StatusOr<int> get10() { return 10; }

art::StatusOr<int> get_error() {
  return art::InvalidArgumentError("Invalid argument");
}

static void test_statur_or() {
  auto ten = get10();
  g_assert(ten.ok());
  g_assert_cmpint(ten.value(), ==, 10);
  g_assert_cmpint(*ten, ==, 10);

  *ten = 20;
  g_assert_cmpint(ten.value(), ==, 20);
  g_assert_cmpint(*ten, ==, 20);

  const auto error = get_error();
  g_assert(!error.ok());
  g_assert_cmpint(error.value_or(100), ==, 100);
  g_assert_cmpint(get_error().value_or(1000), ==, 1000);
}

static void test_get_lines() {
  const auto lines =
      art::GetLines("util/non_exist_file.txt", /*skip_empty=*/true);
  g_assert(!lines.ok());
  ScopedTestFile test_file;
  const auto lines2 = art::GetLines(kFileName, /*skip_empty=*/true);
  g_assert(lines2.ok());
  g_assert_cmpint((*lines2).size(), ==, 2);

  const auto line3 = art::GetLines(kFileName, /*skip_empty=*/false);
  g_assert(line3.ok());
  g_assert_cmpint((*line3).size(), ==, 4);

  // test GetNumLines()
  const auto num_lines =
      art::GetNumLines("__non_exists__", /*skip_empty=*/true);
  g_assert(!num_lines.ok());
  const auto num_lines2 = art::GetNumLines(kFileName, /*skip_empty=*/true);
  g_assert(num_lines2.ok());
  g_assert_cmpint(*num_lines2, ==, 2);
  const auto num_lines3 = art::GetNumLines(kFileName, /*skip_empty=*/false);
  g_assert(num_lines3.ok());
  g_assert_cmpint(*num_lines3, ==, 4);
}

static void test_parse_shortcut() {
  const auto result = art::ParseShortcut("<Ctrl>a");
  g_assert(result.ok());
  const auto &[mods, keyval] = *result;
  g_assert_cmpint(mods, ==, Gdk::CONTROL_MASK);
  g_assert_cmpint(keyval, ==, 'a');

  const auto result2 = art::ParseShortcut("<Ctrl><Shift>a");
  g_assert(result2.ok());
  const auto &[mods2, keyval2] = *result2;
  g_assert_cmpint(mods2, ==, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK);
  g_assert_cmpint(keyval2, ==, 'a');

  const auto result3 = art::ParseShortcut("<Ctrl><Shift><Alt>a");
  g_assert(result3.ok());
  const auto &[mods3, keyval3] = *result3;
  g_assert_cmpint(mods3, ==,
                  Gdk::CONTROL_MASK | Gdk::SHIFT_MASK | Gdk::MOD1_MASK);
  g_assert_cmpint(keyval3, ==, 'a');

  const auto invalid = art::ParseShortcut("<Ctrl><Shift><Alt>");
  g_assert(!invalid.ok());

  const auto invalid2 = art::ParseShortcut("abc");
  g_assert(!invalid2.ok());

  const auto empty = art::ParseShortcut("");
  g_assert(!empty.ok());
}

static void test_list_directory() {
  const auto dir =
      art::ListDirectory(".", /*dir_only=*/false, /*add_hidden=*/false,
                         /*sort_type=*/Gtk::SORT_ASCENDING);
  g_assert(dir.ok());
  const auto &files = *dir;
  g_assert_cmpint(files.size(), >, 0);

  const auto dir_not_exist = art::ListDirectory(
      "____not_exist____", /*dir_only=*/false, /*add_hidden=*/false,
      /*sort_type=*/Gtk::SORT_ASCENDING);
  g_assert(!dir_not_exist.ok());
}

static void test_is_root() {
#ifdef WIN32
  const auto win_root = art::IsRootPath("C:\\");
  g_assert(win_root);
  const auto win_not_root = art::IsRootPath("C:\\Users");
  g_assert(!win_not_root);
#else
  const auto root = art::IsRootPath("/");
  g_assert(root);

  const auto not_root = art::IsRootPath("/home");
  g_assert(!not_root);
#endif
}

static void test_str_start_ends() {
  const auto str = "Hello, World!";
  g_assert(art::StartsWith(str, "Hello"));
  g_assert(art::EndsWith(str, "World!"));
  g_assert(art::StartsWith(str, "Hello, World!"));
  g_assert(art::EndsWith(str, "Hello, World!"));
  g_assert(art::StartsWith(str, ""));
  g_assert(art::EndsWith(str, ""));
  g_assert(!art::StartsWith(str, "World"));
  g_assert(!art::EndsWith(str, "Hello"));
  g_assert(art::StartsWith("", ""));
  g_assert(art::EndsWith("", ""));
  g_assert(!art::StartsWith("", "foo"));
  g_assert(!art::EndsWith("", "foo"));
}

static void test_mmap_file() {
  // create a file
  ScopedTestFile test_file;
  art::MemoryMappedFile file(kFileName);
  // not count the last '\0'
  g_assert_cmpint(file.size(), ==, sizeof(kContent) - 1);
  const char *data = static_cast<const char *>(file.data());
  g_assert(std::equal(data, data + file.size(), kContent));
}

static void test_str_split() {
  const auto str = "a,,b,,c";
  const auto result = art::StrSplit(str, ",", /*skip_empty=*/true);
  g_assert_cmpint(result.size(), ==, 3);

  const auto result2 = art::StrSplit(str, ",", /*skip_empty=*/false);
  g_assert_cmpint(result2.size(), ==, 5);

  g_assert(art::StrSplit("", ",", /*skip_empty=*/true).empty());

  g_assert(art::StrSplit(",", ",", /*skip_empty=*/true).empty());
  g_assert(!art::StrSplit(",", ",", /*skip_empty=*/false).empty());

  g_assert(art::StrSplit("a", ",", /*skip_empty=*/true).size() == 1);
  g_assert(art::StrSplit("a", ",", /*skip_empty=*/false).size() == 1);
}

static void test_span() {
  std::vector<int> vec = {1, 2, 3, 4, 5};
  art::Span<int> span(vec);
  g_assert_cmpint(span.size(), ==, 5);
  g_assert_cmpint(span[0], ==, 1);
  g_assert_cmpint(span.front(), ==, 1);
  g_assert_cmpint(span.back(), ==, 5);
  g_assert_cmpint(span[4], ==, 5);
  g_assert(span.data() == vec.data());
  g_assert(span.begin() == vec.data());
  g_assert(span.end() == vec.data() + vec.size());
  g_assert(span.first(2).size() == 2);
  g_assert(span.first(2)[1] == 2);
  g_assert(span.last(2).size() == 2);
  g_assert(span.last(2)[1] == 5);
  g_assert(span.subspan(1, 3).size() == 3);
  g_assert(span.subspan(1, 3)[0] == 2);
  g_assert(span.subspan(1).size() == 4);
  g_assert(span.subspan(1).front() == 2);
  g_assert(span.subspan(1).back() == 5);
  // pass negative count means counts from the end
  g_assert(span.subspan(-1).size() == 4);
  g_assert(span.subspan(-1).front() == 1);
  g_assert(span.subspan(-1).back() == 4);

  std::vector<Glib::ustring> vec2 = {"a", "b", "c"};
  auto span2 = art::MakeSpan(vec2);
  g_assert_cmpint(span2.size(), ==, 3);
  g_assert(span2[0] == "a");

  std::vector<int> vec_empty;
  auto span_empty = art::MakeSpan(vec_empty);
  g_assert(span_empty.empty());
}

static void test_decompose_path() {
  const auto path = "/usr/local/bin";
  const auto parts = art::DecomposePath(path);
  g_assert_cmpint(parts.size(), ==, 4);
  g_assert_cmpstr(parts[0].c_str(), ==, "/");
  g_assert_cmpstr(parts[1].c_str(), ==, "usr");
  g_assert_cmpstr(parts[2].c_str(), ==, "local");
  g_assert_cmpstr(parts[3].c_str(), ==, "bin");

  // win
  const auto path2 = "C:\\Program Files\\Microsoft";
  const auto parts2 = art::DecomposePath(path2);
  g_assert_cmpint(parts2.size(), ==, 3);
  g_assert_cmpstr(parts2[0].c_str(), ==, "C:\\");
  g_assert_cmpstr(parts2[1].c_str(), ==, "Program Files");
  g_assert_cmpstr(parts2[2].c_str(), ==, "Microsoft");

  // POSIX format on Windows
  const auto path3 = "C:/Program Files/Microsoft";
  const auto parts3 = art::DecomposePath(path3);
  std::cout << art::StrJoin(parts3, ",") << std::endl;
  g_assert_cmpint(parts3.size(), ==, 3);
  g_assert_cmpstr(parts3[0].c_str(), ==, "C:/");
  g_assert_cmpstr(parts3[1].c_str(), ==, "Program Files");
  g_assert_cmpstr(parts3[2].c_str(), ==, "Microsoft");
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, nullptr);
  Glib::init();
  Gio::init();
  Glib::ustring str = "Hello, World!";
  art::Info("Running tests...%s", str);
  art::Info(str);
  g_test_add_func("/StrFormat/Basic", test_str_format);
  g_test_add_func("/StrJoin/Basic", test_str_join);
  g_test_add_func("/StrCat/Basic", test_str_cat);
  g_test_add_func("/Status/Basic", test_status);
  g_test_add_func("/StatusOr/Basic", test_statur_or);
  g_test_add_func("/GetLines/Basic", test_get_lines);
  g_test_add_func("/ParseShortcut/Basic", test_parse_shortcut);
  g_test_add_func("/ListDirectory/Basic", test_list_directory);
  g_test_add_func("/IsRoot/Basic", test_is_root);
  g_test_add_func("/StrStartEnd/Basic", test_str_start_ends);
  g_test_add_func("/MemoryMappedFile/Basic", test_mmap_file);
  g_test_add_func("/StrSplit/Basic", test_str_split);
  g_test_add_func("/Span/Basic", test_span);
  g_test_add_func("/DecomposePath/Basic", test_decompose_path);

  return g_test_run();
}
