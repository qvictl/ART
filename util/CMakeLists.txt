include_directories(${CMAKE_SOURCE_DIR})

# Glob not start with test
file(GLOB UTIL_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cc")

add_library(art_util STATIC ${UTIL_SRCS})

include_directories(art_util PRIVATE ${GTKMM_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS})
# Include directories
target_link_libraries(art_util PRIVATE ${GTKMM_LIBRARIES} ${GLIBMM_LIBRARIES} ${GIOMM_LIBRARIES} ${GIO_LIBRARIES})
