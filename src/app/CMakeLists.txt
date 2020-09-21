configure_file(app_version.h.cmakein app_version.h ESCAPE_QUOTES)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/app_version.h
  DESTINATION include/src/app
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

# Crashpad
set(CRASHPAD_BACKEND_URL "" CACHE STRING "Crashpad backend URL")
set(CRASHPAD_SRC "" CACHE STRING "Path to Crashpad source code")
set(CRASHPAD_BUILD "" CACHE STRING "Path to Crashpad build folder")

if (CRASHPAD_BACKEND_URL
  AND CRASHPAD_SRC
  AND EXISTS "${CRASHPAD_SRC}"
  AND CRASHPAD_BUILD
  AND EXISTS "${CRASHPAD_BUILD}"
  AND (WIN32 OR APPLE)) # LINUX isn't supported for now
  target_compile_definitions(qtcreator PRIVATE
      CRASHPAD_BACKEND_URL="${CRASHPAD_BACKEND_URL}"
      ENABLE_CRASHPAD
      WIN32_LEAN_AND_MEAN) # It comes usually with precompiled header, but at the installer they are disabled
  target_include_directories(qtcreator PRIVATE
      "${CRASHPAD_SRC}"
      "${CRASHPAD_SRC}/third_party/mini_chromium/mini_chromium")

  if (WIN32)
    target_link_libraries(qtcreator PUBLIC ${CRASHPAD_BUILD}/obj/third_party/mini_chromium/mini_chromium/base/base.lib
                                           ${CRASHPAD_BUILD}/obj/util/util.lib
                                           ${CRASHPAD_BUILD}/obj/client/client.lib
                                           advapi32)
    install(FILES ${CRASHPAD_BUILD}/crashpad_handler.exe DESTINATION "${IDE_LIBEXEC_PATH}")

  elseif(APPLE)
    find_library(FWbsm bsm)
    target_link_libraries(qtcreator PUBLIC ${CRASHPAD_BUILD}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
                                           ${CRASHPAD_BUILD}/obj/util/libutil.a
                                           ${CRASHPAD_BUILD}/obj/client/libclient.a
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.child_portServer.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.child_portUser.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.excServer.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.excUser.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.mach_excServer.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.mach_excUser.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.notifyServer.o
                                           ${CRASHPAD_BUILD}/obj/out/Default/gen/util/mach/mig_output.notifyUser.o
                                           ${FWbsm} ${FWAppKit} ${FWIOKit} ${FWSecurity})
    install(FILES ${CRASHPAD_BUILD}/crashpad_handler DESTINATION "${IDE_LIBEXEC_PATH}")

  elseif(UNIX)
    # TODO: Crashpad is not well supported on linux currently
    target_link_libraries(qtcreator PUBLIC ${CRASHPAD_BUILD}/obj/third_party/mini_chromium/mini_chromium/base/libbase.a
                                           ${CRASHPAD_BUILD}/obj/util/libutil.a
                                           ${CRASHPAD_BUILD}/obj/client/libclient.a)
    install(FILES ${CRASHPAD_BUILD}/crashpad_handler DESTINATION "${IDE_LIBEXEC_PATH}")
  endif()
endif()
