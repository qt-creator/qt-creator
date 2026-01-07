configure_file(app_version.h.cmakein app_version.h ESCAPE_QUOTES)

if(NOT IS_ABSOLUTE "${IDE_ICON_PATH}")
  set(IDE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${IDE_ICON_PATH}")
endif()

if (NOT IS_ABSOLUTE ${IDE_LOGO_PATH})
  set(IDE_LOGO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${IDE_LOGO_PATH}")
endif()
configure_file(app_logo.qrc.cmakein app_logo_cmake.qrc)

# This needs to be explicit like this, to avoid issues with the path
# to the binary in generated SBOMs.
set(qtcreator_props "")
if(APPLE)
  list(APPEND qtcreator_props MACOSX_BUNDLE ON)
endif()
if(WIN32)
  list(APPEND qtcreator_props WIN32_EXECUTABLE ON)
endif()
add_qtc_executable(qtcreator
  DEFINES
    IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
    IDE_APP_ID=\"${IDE_APP_ID}\"
  DEPENDS Aggregation ExtensionSystem Qt::Core Qt::Widgets Utils shared_qtsingleapplication app_version
  SOURCES
    main.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/app_logo_cmake.qrc
    ../tools/qtcreatorcrashhandler/crashhandlersetup.cpp ../tools/qtcreatorcrashhandler/crashhandlersetup.h
  PROPERTIES
    ${qtcreator_props}
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

extend_qtc_executable(qtcreator
  CONDITION SHOW_BUILD_DATE
  DEFINES QTC_SHOW_BUILD_DATE
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
  set(RC_COPYRIGHT "${IDE_COPYRIGHT}")
  set(RC_PUBLISHER "${IDE_PUBLISHER}")

  string(REPLACE " " "\\x20" RC_APPLICATION_NAME "${RC_APPLICATION_NAME}")
  string(REPLACE " " "\\x20" RC_PUBLISHER "${RC_PUBLISHER}")
  string(REPLACE " " "\\x20" RC_COPYRIGHT "${RC_COPYRIGHT}")
  string(REPLACE "." "," RC_VERSION "${RC_VERSION}")

  target_compile_definitions(qtcreator PRIVATE
    RC_PUBLISHER=${RC_PUBLISHER}
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
    DEPENDS
      qtcreator.xcassets/qtcreator.appiconset/Contents.json
      qtcreator.xcassets/qtcreator.appiconset/icon_128x128.png
      qtcreator.xcassets/qtcreator.appiconset/icon_128x128@2x.png
      qtcreator.xcassets/qtcreator.appiconset/icon_16x16.png
      qtcreator.xcassets/qtcreator.appiconset/icon_16x16@2x.png
      qtcreator.xcassets/qtcreator.appiconset/icon_256x256.png
      qtcreator.xcassets/qtcreator.appiconset/icon_256x256@2x.png
      qtcreator.xcassets/qtcreator.appiconset/icon_32x32.png
      qtcreator.xcassets/qtcreator.appiconset/icon_32x32@2x.png
      qtcreator.xcassets/qtcreator.appiconset/icon_512x512.png
      qtcreator.xcassets/qtcreator.appiconset/icon_512x512@2x.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_128x128.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_128x128@2x.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_16x16.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_16x16@2x.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_256x256.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_256x256@2x.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_32x32.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_32x32@2x.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_512x512.png
      qtcreator.xcassets/qtcreator-project.iconset/icon_512x512@2x.png
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

if ((NOT WIN32) AND (NOT APPLE))
  # install logo
  foreach(size 16 24 32 48 64 128 256 512)
    install(
      FILES ${IDE_LOGO_PATH}/images/logo/${size}/QtProject-${IDE_ID}.png
      DESTINATION share/icons/hicolor/${size}x${size}/apps
    )
  endforeach()
endif()
