configure_file(app_version.h.cmakein app_version.h ESCAPE_QUOTES)

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

add_custom_command(TARGET qtcreator POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
          "${PROJECT_SOURCE_DIR}/share/qtcreator"
          "${PROJECT_BINARY_DIR}/${IDE_DATA_PATH}")
