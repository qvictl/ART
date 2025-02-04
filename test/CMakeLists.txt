# Enable testing
enable_testing()

# Define the test executable
add_executable(test_all_util test_all_util.cc)

# Include directories
target_include_directories(test_all_util PRIVATE ${GTKMM_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS})

# Link the test executable to the library and dependencies
target_link_libraries(test_all_util PRIVATE art_util ${GTKMM_LIBRARIES} ${GLIBMM_LIBRARIES})

# Add test to CTest
add_test(NAME StrFormatTest COMMAND test_all_util)
