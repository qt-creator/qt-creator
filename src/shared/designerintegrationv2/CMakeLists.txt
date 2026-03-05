if (NOT TARGET Qt::Designer)
  return()
endif()

add_qtc_library(designerintegrationv2 STATIC
  DEPENDS Qt::Designer Qt::Widgets
  PUBLIC_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}"
  SOURCES
    formresizer.cpp formresizer.h
    sizehandlerect.cpp sizehandlerect.h
    widgethostconstants.h
    widgethost.cpp widgethost.h
)

if (QTC_STATIC_BUILD)
  get_target_property(_designerType Qt::Designer TYPE)
  if (${_designerType} STREQUAL "STATIC_LIBRARY")
    extend_qtc_target(designerintegrationv2 PUBLIC_DEFINES QT_DESIGNER_STATIC)
  endif()
endif()
