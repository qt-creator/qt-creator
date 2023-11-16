add_qtc_library(Nanotrace
  BUILD_DEFAULT OFF
  PUBLIC_DEFINES NANOTRACE_ENABLED
  SOURCES
    nanotraceglobals.h
    nanotrace.cpp nanotrace.h
    nanotracehr.cpp nanotracehr.h
  PUBLIC_DEPENDS Qt::Core Qt::Gui
  PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)
