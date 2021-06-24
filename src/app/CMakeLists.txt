configure_file(app_version.h.cmakein app_version.h ESCAPE_QUOTES)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/app_version.h
  DESTINATION ${IDE_HEADER_INSTALL_PATH}/src/app
  COMPONENT Devel EXCLUDE_FROM_ALL
)

add_qtc_executable(qtcreator
  DEFINES IDE_LIBRARY_BASENAME=\"${IDE_LIBRARY_BASE_PATH}\"
  DEPENDS Aggregation ExtensionSystem Qt5::Core Qt5::Widgets Utils shared_qtsingleapplication app_version
  SOURCES
    main.cpp
    ../tools/qtcreatorcrashhandler/crashhandlersetup.cpp ../tools/qtcreatorcrashhandler/crashhandlersetup.h
  PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    OUTPUT_NAME "${IDE_APP_TARGET}"
  DESTINATION "${IDE_APP_PATH}"
  QTC_RUNNABLE
)

if (NOT TARGET qtcreator)
  return()
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
      "${CMAKE_CURRENT_SOURCE_DIR}/qtcreator.xcassets"
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
