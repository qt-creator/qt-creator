add_qtc_library(DesignSystem STATIC
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  DEPENDS
    Qt::Core Qt::Widgets QmlDesignerCore TextEditorSupport
  SOURCES
    dsconstants.h
    dsstore.h dsstore.cpp
    dsthemegroup.h dsthemegroup.cpp
    dsthememanager.h dsthememanager.cpp
)

extend_qtc_library(DesignSystem
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)
