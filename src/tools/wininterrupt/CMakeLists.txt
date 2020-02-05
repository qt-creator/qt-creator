if (NOT WIN32)
  return()
endif()

set(Arch 32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(Arch 64)
endif()

add_qtc_executable(win${Arch}interrupt
  COMPONENT wininterrupt
  SOURCES wininterrupt.c
)
