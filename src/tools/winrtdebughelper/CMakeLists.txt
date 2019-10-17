if (NOT WIN32)
  return()
endif()

add_qtc_executable(winrtdebughelper
  SOURCES
    winrtdebughelper.cpp
)
