if (NOT TARGET Qt5::Help)
  # Do not built without Qt5::Help being available
  return()
endif()

set(PLUGIN_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/plugins")

add_qtc_library(shared_help STATIC
  DEPENDS Utils PUBLIC Qt5::Help Qt5::PrintSupport Qt5::Widgets
  PUBLIC_INCLUDES
    "${PLUGIN_SOURCE_DIR}/help"
    "${PLUGIN_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/shared_help_autogen/include"
    "${CMAKE_CURRENT_BINARY_DIR}/"
    "${CMAKE_CURRENT_SOURCE_DIR}"
  SOURCES
    bookmarkdialog.ui
    bookmarkmanager.cpp bookmarkmanager.h
    contentwindow.cpp contentwindow.h
    filternamedialog.cpp filternamedialog.h filternamedialog.ui
    helpicons.h
    indexwindow.cpp indexwindow.h
    topicchooser.cpp topicchooser.h topicchooser.ui
)
