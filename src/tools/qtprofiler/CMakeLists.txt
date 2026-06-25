option(BUILD_EXECUTABLE_QTPROFILER_STANDALONE
       "Creates an install target for standalone qtprofiler"
       OFF)

# On macOS qtprofiler is built as its own application bundle next to
# "Qt Creator.app" (instead of being buried inside its libexec directory), so
# it behaves like a standalone application with its own icon and Info.plist.
# The bundle (and its executable) use the "Qt Profiler" display name, while the
# target/identifier stays "qtprofiler".
set(qtprofiler_bundle_name "Qt Profiler")
set(qtprofiler_props "")
set(qtprofiler_destination "${IDE_LIBEXEC_PATH}")
if (APPLE)
  list(APPEND qtprofiler_props MACOSX_BUNDLE ON OUTPUT_NAME "${qtprofiler_bundle_name}")
  set(qtprofiler_destination "${IDE_APP_PATH}")
endif()

add_qtc_executable(qtprofiler
  CONDITION TARGET Profiler
  SKIP_INSTALL
  DEPENDS app_version Profiler CommonTraceFormat Tracing
  PROPERTIES ${qtprofiler_props}
  DESTINATION "${qtprofiler_destination}"
  SOURCES
    mainsidebar.cpp mainsidebar.h
    qtprofilerinit.cpp qtprofilerinit.h
    qtprofilermain.cpp
    qtprofilerrpc.cpp qtprofilerrpc.h
    qtprofilersettings.cpp qtprofilersettings.h
    qtprofilerwindow.h qtprofilerwindow.cpp
    recordingpage.cpp recordingpage.h
    welcomepage.cpp welcomepage.h
    schema/api.h
)

# Turn it into a proper macOS application bundle: attach the Info.plist and the
# app icon, and make sure both end up in the build tree and in the install tree.
if (APPLE AND TARGET qtprofiler)
  if (CMAKE_OSX_DEPLOYMENT_TARGET)
    set(MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()

  set_target_properties(qtprofiler PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
  )

  # Copies the icon into qtprofiler.app/Contents/Resources in the build tree.
  set(qtprofiler_icon "${CMAKE_CURRENT_SOURCE_DIR}/macos/qtprofiler.icns")
  target_sources(qtprofiler PRIVATE "${qtprofiler_icon}")
  set_source_files_properties("${qtprofiler_icon}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
  )

  # Copy translations to build dir. Themes are compiled into the app with
  # qt_add_resources below
  qtc_output_binary_dir(_output_binary_dir)
  qtc_copy_to_builddir(copy_qtprofiler_resources
    DIRECTORIES "${_output_binary_dir}/${IDE_DATA_PATH}/translations"
    DESTINATION "${qtprofiler_bundle_name}.app/Contents/Resources/translations"
  )
  # The generated translations must exist in the build tree before we copy them.
  if (TARGET generate_qm_files)
    add_dependencies(copy_qtprofiler_resources generate_qm_files)
  endif()
endif()

qtc_add_resources(qtprofiler "schema"
  PREFIX "/qtprofiler"
  BASE "."
  FILES
    schema/qtprofilerapi.json.schema
)

# Compile the editor themes (and every file they include) straight into the
# binary, so the application loads them from ":/qtprofiler/themes" instead of a
# themes directory on disk. The includes are resolved relative to the theme
# file, so they must live next to it under the same resource prefix.
if (TARGET qtprofiler)
  set(qtprofiler_themes_dir "${PROJECT_SOURCE_DIR}/share/qtcreator/themes")
  qt_add_resources(qtprofiler "themes"
    PREFIX "/qtprofiler/themes"
    BASE "${qtprofiler_themes_dir}"
    FILES
      "${qtprofiler_themes_dir}/dark-2024.creatortheme"
      "${qtprofiler_themes_dir}/light-2024.creatortheme"
      "${qtprofiler_themes_dir}/dark.figmatokens"
      "${qtprofiler_themes_dir}/light.figmatokens"
      "${qtprofiler_themes_dir}/primitive-colors.inc"
      "${qtprofiler_themes_dir}/2024.tokenmapping"
      "${qtprofiler_themes_dir}/ds.tokenmapping"
      "${qtprofiler_themes_dir}/qmldesigner-dark.inc"
      "${qtprofiler_themes_dir}/qmldesigner-light.inc"
  )
endif()

# The call-stack sampler uses task_for_pid(), which requires the binary to carry
# the debugger entitlement. Re-sign (ad-hoc) after each build so the freshly
# linked binary keeps it. Without this the sampler fails at attach time.
if (APPLE AND TARGET qtprofiler)
  add_custom_command(TARGET qtprofiler POST_BUILD
    COMMAND codesign --sign - --force
            --entitlements "${QtCreator_SOURCE_DIR}/dist/installer/mac/Qt Profiler.entitlements"
            "$<TARGET_BUNDLE_DIR:qtprofiler>"
    VERBATIM
    COMMENT "Codesigning qtprofiler with the debugger entitlement"
  )
endif()

if (NOT TARGET qtprofiler OR NOT BUILD_EXECUTABLE_QTPROFILER_STANDALONE)
  return()
endif()

create_standalone_install_component(qtprofiler
  TARGET qtprofiler
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
    translations
)
