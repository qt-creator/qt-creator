option(BUILD_EXECUTABLE_QMLTRACEVIEWER_STANDALONE
       "Creates an install target for standalone qmltraceviewer"
       OFF)

add_qtc_executable(qmltraceviewer
  CONDITION TARGET QmlProfiler
  DEPENDS app_version QmlProfiler
  SOURCES
    qmltraceviewerinit.cpp qmltraceviewerinit.h
    qmltraceviewermain.cpp
    qmltraceviewerrpc.cpp qmltraceviewerrpc.h
    qmltraceviewersettings.cpp qmltraceviewersettings.h
    qmltraceviewerwindow.h qmltraceviewerwindow.cpp
    schema/api.h
)

qtc_add_resources(qmltraceviewer "schema"
  PREFIX "/qmltraceviewer"
  BASE "."
  FILES
    schema/qmltraceviewerapi.json.schema
)

if (NOT TARGET qmltraceviewer OR NOT BUILD_EXECUTABLE_QMLTRACEVIEWER_STANDALONE)
  return()
endif()

create_standalone_install_component(standalone_qmltraceviewer
  TARGET qmltraceviewer
  BUNDLE_PLIST_TEMPLATE ${CMAKE_CURRENT_LIST_DIR}/Info.plist.in
  PLUGINS
    platforminputcontexts
    platforms
    platformthemes
    styles
    wayland-decoration-client
    wayland-graphics-integration-client
    wayland-shell-integration
    xcbglintegrations
  IMPORTS
    QML
    Qt
    QtCore
    QtNetwork
    QtQml
    QtQuick
    QtWayland
  RESOURCES
    themes
    translations
)
