if (NOT TARGET Qt::Help)
  # Do not built without Qt::Help being available
  return()
endif()

set(PLUGIN_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/plugins")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
unset(autogen_suffix)
if (isMultiConfig)
  set(autogen_suffix "_$<CONFIG>")
endif()

add_qtc_library(shared_help STATIC
  DEPENDS Utils Core PUBLIC Qt::Help Qt::PrintSupport Qt::Widgets
  PUBLIC_INCLUDES
    "${PLUGIN_SOURCE_DIR}/help"
    "${PLUGIN_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/shared_help_autogen/include${autogen_suffix}"
    "${CMAKE_CURRENT_BINARY_DIR}/"
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    bookmarkmanager.cpp bookmarkmanager.h
    contentwindow.cpp contentwindow.h
    helpicons.h
    indexwindow.cpp indexwindow.h
    topicchooser.cpp topicchooser.h
)
