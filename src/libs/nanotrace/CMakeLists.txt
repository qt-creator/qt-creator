add_qtc_library(Nanotrace
  BUILD_DEFAULT OFF
  DEFINES NANOTRACE_LIBRARY
  PUBLIC_DEFINES NANOTRACE_ENABLED
  SOURCES
    nanotraceglobals.h
    nanotrace.cpp nanotrace.h
    nanotracehr.cpp nanotracehr.h
  PUBLIC_DEPENDS Qt::Core
  PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)
