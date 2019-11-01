if (NOT WIN32)
  return()
endif()

add_qtc_executable(qtcdebugger
  DEPENDS Qt5::Widgets registryaccess psapi app_version
  SOURCES
    main.cpp
  PROPERTIES WIN32_EXECUTABLE ON
)
