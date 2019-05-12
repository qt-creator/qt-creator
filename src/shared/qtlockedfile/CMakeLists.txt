if (WIN32)
  set(OS_SOURCES qtlockedfile_win.cpp)
else()
  set(OS_SOURCES qtlockedfile_unix.cpp)
endif()

add_library(shared_qtlockedfile STATIC ${OS_SOURCES} qtlockedfile.cpp qtlockedfile.h)
target_link_libraries(shared_qtlockedfile Qt5::Core)
target_include_directories(shared_qtlockedfile PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
if (WIN32)
  target_compile_definitions(shared_qtlockedfile PRIVATE
      "QT_QTLOCKEDFILE_EXPORT=__declspec(dllexport)" _UNICODE UNICODE)
endif()
