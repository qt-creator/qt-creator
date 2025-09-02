add_qtc_library(qtcZLib
  STATIC
  SKIP_AUTOMOC
  SOURCES
    src/adler32.c
    src/compress.c
    src/crc32.c
    src/crc32.h
    src/deflate.c
    src/deflate.h
    src/gzclose.c
    src/gzguts.h
    src/gzlib.c
    src/gzread.c
    src/gzwrite.c
    src/infback.c
    src/inffast.c
    src/inffast.h
    src/inffixed.h
    src/inflate.c
    src/inflate.h
    src/inftrees.c
    src/inftrees.h
    src/trees.c
    src/trees.h
    src/uncompr.c
    src/zconf.h
    src/zlib.h
    src/zutil.c
    src/zutil.h
  PUBLIC_INCLUDES
    src
)

if (MSVC)
  target_compile_options(qtcZLib PRIVATE /wd4996)
endif()
