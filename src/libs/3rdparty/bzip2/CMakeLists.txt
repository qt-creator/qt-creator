add_qtc_library(qtcBZip2
  STATIC
  SKIP_AUTOMOC
  SOURCES
    bzlib.h
    blocksort.c
    huffman.c
    crctable.c
    randtable.c
    compress.c
    decompress.c
    bzlib.c
  PUBLIC_INCLUDES
    .
)

IF (NOT MSVC)
  # Disable warnings
  target_compile_options(qtcBZip2 PRIVATE "-Wimplicit-fallthrough=0" "-Wno-unused-parameter")
endif()
