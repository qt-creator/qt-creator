add_qtc_library(libvterm STATIC
  PUBLIC_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/include
  PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
  SOURCES
    src/encoding.c
    src/fullwidth.inc
    src/keyboard.c
    src/mouse.c
    src/parser.c
    src/pen.c
    src/rect.h
    src/screen.c
    src/state.c
    src/unicode.c
    src/utf8.h
    src/vterm.c
    src/vterm_internal.h
)
