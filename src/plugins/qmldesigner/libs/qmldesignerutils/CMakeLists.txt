add_qtc_library(QmlDesignerUtils STATIC
  DEPENDS
    Qt::Gui Utils Qt::QmlPrivate
  PUBLIC_INCLUDES ${CMAKE_CURRENT_LIST_DIR}
  PUBLIC_COMPILE_OPTIONS
    $<$<COMPILE_LANG_AND_ID:CXX,Clang>:-Wno-unneeded-internal-declaration>
  SOURCES
    asset.cpp asset.h
    designeralgorithm.h
    memory.h
    hdrimage.cpp hdrimage.h
    ktximage.cpp ktximage.h
    imageutils.cpp imageutils.h
    qmldesignerutils_global.h
    version.cpp version.h
    maputils.h
    stringutils.cpp stringutils.h
    importutils.cpp importutils.h
)

extend_qtc_library(QmlDesignerUtils
    CONDITION ENABLE_COMPILE_WARNING_AS_ERROR
    PROPERTIES COMPILE_WARNING_AS_ERROR ON
    PUBLIC_COMPILE_OPTIONS
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,GNU>:-Wno-error=maybe-uninitialized>
)
