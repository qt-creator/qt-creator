add_library(designerintegrationv2 STATIC
  formresizer.cpp formresizer.h
  sizehandlerect.cpp sizehandlerect.h
  widgethostconstants.h
  widgethost.cpp widgethost.h
)
target_link_libraries(designerintegrationv2 PUBLIC Qt5::Designer)
target_include_directories(designerintegrationv2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
