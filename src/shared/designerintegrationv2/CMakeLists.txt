if (NOT TARGET Qt5::Designer)
  return()
endif()

add_qtc_library(designerintegrationv2 STATIC
  DEPENDS Qt5::Designer Qt5::Widgets
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    formresizer.cpp formresizer.h
    sizehandlerect.cpp sizehandlerect.h
    widgethostconstants.h
    widgethost.cpp widgethost.h
)
