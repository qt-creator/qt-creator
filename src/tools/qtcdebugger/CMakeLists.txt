if (NOT WIN32)
  return()
endif()

add_qtc_executable(qtcdebugger
  DEPENDS Qt::Widgets registryaccess psapi app_version
  SOURCES
    main.cpp
  PROPERTIES WIN32_EXECUTABLE ON
)

qtc_add_version_resource(qtcdebugger "${IDE_DISPLAY_NAME} Debug Dispatcher")
