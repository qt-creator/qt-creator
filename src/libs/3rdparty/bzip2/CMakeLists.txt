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

if (MSVC)
  target_compile_options(qtcBZip2 PUBLIC /wd4244 /wd4267 /wd4996)
else()
  # Disable warnings
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(qtcBZip2 PRIVATE "-Wno-unused-parameter")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(qtcBZip2 PRIVATE "-Wimplicit-fallthrough=0" "-Wno-unused-parameter")
  endif()
endif()
