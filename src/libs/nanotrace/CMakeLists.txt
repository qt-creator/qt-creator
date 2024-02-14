add_qtc_library(Nanotrace
  PUBLIC_DEFINES NANOTRACE_ENABLED
  SOURCES
    nanotraceglobals.h
    nanotrace.cpp nanotrace.h
  PUBLIC_DEPENDS Qt::Core Qt::Gui
  PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

option(DESIGNSTUDIO_USE_NANOTRACE
       "Enables collecting performance data with nanotrace for Design Studio" OFF)
extend_qtc_library(Nanotrace
  CONDITION DESIGN_STUDIO_USE_NANOTRACE
  PUBLIC_DEFINES NANOTRACE_DESIGNSTUDIO_ENABLED
)

option(NANOTRACEHR_ENABLED "Enables collecting high resolution performance data" OFF)
extend_qtc_library(Nanotrace
  SOURCES
    nanotracehr.cpp nanotracehr.h
)
