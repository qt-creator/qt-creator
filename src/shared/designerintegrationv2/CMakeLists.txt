if (NOT TARGET Qt5::Designer)
  return()
endif()

add_qtc_library(designerintegrationv2 STATIC
  DEPENDS Qt5::Designer Qt5::Widgets
  SOURCES
  formresizer.cpp formresizer.h
  sizehandlerect.cpp sizehandlerect.h
  widgethostconstants.h
  widgethost.cpp widgethost.h
)
