option(BUILD_EXECUTABLE_QMLTRACEVIEWER_STANDALONE
       "Creates an install target for standalone qmltraceviewer"
       OFF)

add_qtc_executable(qmltraceviewer
  CONDITION TARGET QmlProfiler
  DEPENDS app_version QmlProfiler CommonTraceFormat
  SOURCES
    qmltraceviewerinit.cpp qmltraceviewerinit.h
    qmltraceviewermain.cpp
    qmltraceviewerrpc.cpp qmltraceviewerrpc.h
    qmltraceviewersettings.cpp qmltraceviewersettings.h
    qmltraceviewerwindow.h qmltraceviewerwindow.cpp
    mainsidebar.cpp mainsidebar.h
    recordingpage.cpp recordingpage.h
    welcomepage.cpp welcomepage.h
    schema/api.h
)

qtc_add_resources(qmltraceviewer "schema"
  PREFIX "/qmltraceviewer"
  BASE "."
  FILES
    schema/qmltraceviewerapi.json.schema
)

# The call-stack sampler uses task_for_pid(), which requires the binary to carry
# the debugger entitlement. Re-sign (ad-hoc) after each build so the freshly
# linked binary keeps it. Without this the sampler fails at attach time.
if (APPLE AND TARGET qmltraceviewer)
  add_custom_command(TARGET qmltraceviewer POST_BUILD
    COMMAND codesign --sign - --force
            --entitlements "${CMAKE_CURRENT_LIST_DIR}/qmltraceviewer.entitlements"
            "$<TARGET_FILE:qmltraceviewer>"
    VERBATIM
    COMMENT "Codesigning qmltraceviewer with the debugger entitlement"
  )
endif()

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
    Qt
    QtCore
    QtNetwork
    QtQml
    QtWayland
  RESOURCES
    themes
    translations
)
