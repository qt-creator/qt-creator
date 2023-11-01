configure_file(app_version.h.cmakein app_version.h ESCAPE_QUOTES)

if(NOT IS_ABSOLUTE "${IDE_ICON_PATH}")
  set(IDE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${IDE_ICON_PATH}")
endif()

if (NOT IS_ABSOLUTE ${IDE_LOGO_PATH})
  set(IDE_LOGO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${IDE_LOGO_PATH}")
endif()
configure_file(app_logo.qrc.cmakein app_logo_cmake.qrc)

add_qtc_executable(qtcreator
  DEFINES IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
  DEPENDS Aggregation ExtensionSystem Qt::Core Qt::Widgets Utils shared_qtsingleapplication app_version
  SOURCES
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/app_logo_cmake.qrc
    ../tools/qtcreatorcrashhandler/crashhandlersetup.cpp ../tools/qtcreatorcrashhandler/crashhandlersetup.h
  PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    OUTPUT_NAME "${IDE_APP_TARGET}"
  DESTINATION "${IDE_APP_PATH}"
  QTC_RUNNABLE
)

extend_qtc_executable(qtcreator
  CONDITION APPLE
  DEPENDS ${FWFoundation}
  SOURCES
    main_mac.mm
)

if (NOT TARGET qtcreator)
  return()
endif()

if (QTC_STATIC_BUILD)
  set(plugins_to_import "")
  set(source_file_content "#include <QtPlugin>\n")
  foreach(plugin IN LISTS __QTC_PLUGINS)
    if(TARGET ${plugin})
      list(APPEND plugins_to_import "${plugin}")
      # TODO ${plugin}Plugin is not correct for all plugins as the class name
      get_target_property(plugin_class ${plugin} QTC_PLUGIN_CLASS_NAME)
      string(APPEND source_file_content "Q_IMPORT_PLUGIN(${plugin_class})\n")
    endif()
  endforeach()

  string(APPEND source_file_content
    "struct ResourcesInitializer {\n"
    "  ResourcesInitializer() {\n")
  foreach(resource_file IN LISTS __QTC_RESOURCE_FILES)
     string(APPEND source_file_content "    Q_INIT_RESOURCE(${resource_file});\n")
  endforeach()
  string(APPEND source_file_content
    "  }\n"
    "} g_resources_initializer;\n")

  file(GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/plugin_imports.cpp"
    CONTENT "${source_file_content}"
  )
  extend_qtc_target(qtcreator
    DEPENDS ${plugins_to_import}
    SOURCES "${CMAKE_CURRENT_BINARY_DIR}/plugin_imports.cpp"
  )

  if (TARGET QmlDesigner)
    qt6_import_qml_plugins(qtcreator
      PATH_TO_SCAN "${CMAKE_CURRENT_SOURCE_DIR}/../../share/qtcreator/qmldesigner")
  endif()
endif()

if (WIN32)
  set(RC_APPLICATION_NAME "${IDE_DISPLAY_NAME}")
  set(RC_VERSION "${IDE_VERSION}.0")
  set(RC_VERSION_STRING "${IDE_VERSION_DISPLAY}")
  set(RC_COPYRIGHT "2008-${IDE_COPYRIGHT_YEAR} The Qt Company Ltd")

  string(REPLACE " " "\\x20" RC_APPLICATION_NAME "${RC_APPLICATION_NAME}")
  string(REPLACE " " "\\x20" RC_COPYRIGHT "${RC_COPYRIGHT}")
  string(REPLACE "." "," RC_VERSION "${RC_VERSION}")

  target_compile_definitions(qtcreator PRIVATE
    RC_APPLICATION_NAME=${RC_APPLICATION_NAME}
    RC_VERSION=${RC_VERSION}
    RC_VERSION_STRING=${RC_VERSION_STRING}
    RC_COPYRIGHT=${RC_COPYRIGHT}
    RC_ICON_PATH=${IDE_ICON_PATH}
  )

  target_sources(qtcreator PRIVATE qtcreator.rc)
endif()

if (APPLE)
  if (CMAKE_OSX_DEPLOYMENT_TARGET)
    set(MACOSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()
  set_target_properties(qtcreator PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/app-Info.plist"
  )
  # icon
  add_custom_command(
    OUTPUT qtcreator.icns qtcreator-project.icns
    COMMAND
      xcrun actool --app-icon qtcreator --output-partial-info-plist "${CMAKE_CURRENT_BINARY_DIR}/qtcreator.Info.plist"
      --platform macosx --minimum-deployment-target 10.7 --compile "${CMAKE_CURRENT_BINARY_DIR}"
      "${IDE_ICON_PATH}/qtcreator.xcassets"
      > /dev/null
    BYPRODUCTS qtcreator.Info.plist
    VERBATIM
  )
  target_sources(qtcreator
    PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}/qtcreator.icns"
      "${CMAKE_CURRENT_BINARY_DIR}/qtcreator-project.icns"
  )
  qtc_copy_to_builddir(copy_icns
    DESTINATION ${IDE_DATA_PATH}
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/qtcreator.icns
      ${CMAKE_CURRENT_BINARY_DIR}/qtcreator-project.icns
  )
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/qtcreator.icns
      ${CMAKE_CURRENT_BINARY_DIR}/qtcreator-project.icns
    DESTINATION ${IDE_DATA_PATH}
  )
endif()

option(QTC_FORCE_XCB "Enables that Qt Creator forces XCB on Linux if QT_QPA_PLATFORM is not set." OFF)
extend_qtc_executable(qtcreator
  CONDITION QTC_FORCE_XCB
  DEFINES
    QTC_FORCE_XCB
)

extend_qtc_executable(qtcreator
  CONDITION BUILD_WITH_CRASHPAD
  DEFINES
    CRASHPAD_BACKEND_URL="${CRASHPAD_BACKEND_URL}"
    ENABLE_CRASHPAD
    WIN32_LEAN_AND_MEAN # It comes usually with precompiled header, but at the installer they are disabled
  DEPENDS Crashpad::Crashpad
)
if(BUILD_WITH_CRASHPAD)
  install(
    PROGRAMS ${CRASHPAD_BIN_DIR}/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}
    DESTINATION "${IDE_LIBEXEC_PATH}"
  )
endif()

if ((NOT WIN32) AND (NOT APPLE))
  # install logo
  foreach(size 16 24 32 48 64 128 256 512)
    install(
      FILES ${IDE_LOGO_PATH}/images/logo/${size}/QtProject-qtcreator.png
      DESTINATION share/icons/hicolor/${size}x${size}/apps
    )
  endforeach()
endif()
