add_qtc_library(Nanotrace
  PUBLIC_DEFINES NANOTRACE_ENABLED
  PUBLIC_COMPILE_OPTIONS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd5030>
  SOURCES
    nanotraceglobals.h
    nanotrace.cpp nanotrace.h
    nanotracehr.cpp nanotracehr.h
    staticstring.h
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
