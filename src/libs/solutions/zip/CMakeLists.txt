add_qtc_library(Zip
  DEPENDS Qt::Core
  DEFINES ZIP_LIB
  SOURCES
    zip.cpp
    zipreader.h
    zipwriter.h
    zip_global.h
)

find_package(ZLIB QUIET)
if(ZLIB_FOUND)
  set(QTC_USE_SYSTEM_ZLIB_DEFAULT ON)
else()
  set(QTC_USE_SYSTEM_ZLIB_DEFAULT OFF)
endif()
option(QTC_USE_SYSTEM_ZLIB "Use system zlib for Zip" ${QTC_USE_SYSTEM_ZLIB_DEFAULT})

if (QTC_USE_SYSTEM_ZLIB)
  extend_qtc_library(Zip
    FEATURE_INFO "System zlib"
    DEPENDS ZLIB::ZLIB
  )
else()
  extend_qtc_library(Zip
    FEATURE_INFO "Integrated zlib"
    SOURCES_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/zlib/src
    INCLUDES
      ${CMAKE_CURRENT_LIST_DIR}/../../3rdparty/zlib/src
    SKIP_AUTOMOC
    SOURCES
      adler32.c
      compress.c
      crc32.c
      crc32.h
      deflate.c
      deflate.h
      gzclose.c
      gzguts.h
      gzlib.c
      gzread.c
      gzwrite.c
      infback.c
      inffast.c
      inffast.h
      inffixed.h
      inflate.c
      inflate.h
      inftrees.c
      inftrees.h
      trees.c
      trees.h
      uncompr.c
      zconf.h
      zlib.h
      zutil.c
      zutil.h
  )
  if(MSVC)
    target_compile_options(Zip PUBLIC /wd4996)
  endif()
endif()
