add_qtc_library(Nanotrace
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

target_compile_options(Nanotrace PUBLIC $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/wd5030>)

option(DESIGNSTUDIO_USE_NANOTRACE
       "Enables collecting performance data with nanotrace for Design Studio" OFF)
extend_qtc_library(Nanotrace
  CONDITION DESIGN_STUDIO_USE_NANOTRACE
  PUBLIC_DEFINES NANOTRACE_DESIGNSTUDIO_ENABLED
)
