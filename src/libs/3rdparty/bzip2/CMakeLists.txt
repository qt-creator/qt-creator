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

target_compile_options(qtcBZip2 PRIVATE
  $<$<OR:$<COMPILE_LANG_AND_ID:C,Clang,AppleClang>,$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>>:
    -Wno-unused-parameter -Wno-unused-variable>
  $<$<OR:$<COMPILE_LANG_AND_ID:C,GNU>,$<COMPILE_LANG_AND_ID:CXX,GNU>>:
    -Wno-unused-parameter -Wno-unused-variable -Wimplicit-fallthrough=0>
  $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:
    /wd4100 /wd4189>
)