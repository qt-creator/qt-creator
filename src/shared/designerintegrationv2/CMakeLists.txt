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
  # version-less target Qt::Designer is an interface library that links to QtX::Designer
  get_target_property(_designer Qt::Designer INTERFACE_LINK_LIBRARIES)
  get_target_property(_designerType ${_designer} TYPE)
  if (${_designerType} STREQUAL "STATIC_LIBRARY")
    extend_qtc_target(designerintegrationv2 PUBLIC_DEFINES QT_DESIGNER_STATIC)
  endif()
endif()
