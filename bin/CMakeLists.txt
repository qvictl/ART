# Define the test executable
add_executable(ui_base_example ui_base_example.cc)

# Include directories
target_include_directories(ui_base_example PRIVATE ${GTKMM_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS})

# Link the test executable to the library and dependencies
target_link_libraries(ui_base_example PRIVATE art_util ui_base ${GTKMM_LIBRARIES} ${GLIBMM_LIBRARIES})
