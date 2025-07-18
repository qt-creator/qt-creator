add_qtc_library(AstCheck STATIC
  DEPENDS
    Qt::Core QmlJS Utils
  SOURCES
    astcheck.cpp astcheck.h
    astutils.cpp astutils.h
)

extend_qtc_library(AstCheck
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)
