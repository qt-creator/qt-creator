add_library(libvterm STATIC
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

target_include_directories(libvterm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(libvterm
  PROPERTIES
    QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
    POSITION_INDEPENDENT_CODE ON 
)
