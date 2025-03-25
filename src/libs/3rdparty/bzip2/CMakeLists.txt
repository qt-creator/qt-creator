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
  IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(qtcBZip2 PRIVATE "-Wno-unused-parameter")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(qtcBZip2 PRIVATE "-Wimplicit-fallthrough=0" "-Wno-unused-parameter")
  endif()
endif()
