find_package(GoogleBenchmark MODULE)

if (NOT TARGET Googletest)
  return()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(IMPLICIT_DEPENDS Qt::Test)

add_subdirectory(tests)
add_subdirectory(tools)
