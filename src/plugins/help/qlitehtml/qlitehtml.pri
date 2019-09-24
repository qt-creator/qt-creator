exists($$PWD/litehtml/CMakeLists.txt) {
    LH_SRC = $$PWD/litehtml
    GB_SRC = $$PWD/litehtml/src/gumbo

    # gumbo
    SOURCES += \
        $$GB_SRC/attribute.c \
        $$GB_SRC/char_ref.c \
        $$GB_SRC/error.c \
        $$GB_SRC/parser.c \
        $$GB_SRC/string_buffer.c \
        $$GB_SRC/string_piece.c \
        $$GB_SRC/tag.c \
        $$GB_SRC/tokenizer.c \
        $$GB_SRC/utf8.c \
        $$GB_SRC/util.c \
        $$GB_SRC/vector.c

    HEADERS += \
        $$GB_SRC/attribute.h \
        $$GB_SRC/char_ref.h \
        $$GB_SRC/error.h \
        $$GB_SRC/gumbo.h \
        $$GB_SRC/insertion_mode.h \
        $$GB_SRC/parser.h \
        $$GB_SRC/string_buffer.h \
        $$GB_SRC/string_piece.h \
        $$GB_SRC/tag_enum.h \
        $$GB_SRC/tag_gperf.h \
        $$GB_SRC/tag_sizes.h \
        $$GB_SRC/tag_strings.h \
        $$GB_SRC/token_type.h \
        $$GB_SRC/tokenizer.h \
        $$GB_SRC/tokenizer_states.h \
        $$GB_SRC/utf8.h \
        $$GB_SRC/util.h \
        $$GB_SRC/vector.h

    win32 {
        HEADERS += \
            $$GB_SRC/visualc/include/strings.h
        INCLUDEPATH *= $$GB_SRC/visualc/include
    }

    # litehtml
    SOURCES += \
        $$LH_SRC/src/background.cpp \
        $$LH_SRC/src/box.cpp \
        $$LH_SRC/src/context.cpp \
        $$LH_SRC/src/css_length.cpp \
        $$LH_SRC/src/css_selector.cpp \
        $$LH_SRC/src/document.cpp \
        $$LH_SRC/src/el_anchor.cpp \
        $$LH_SRC/src/el_base.cpp \
        $$LH_SRC/src/el_before_after.cpp \
        $$LH_SRC/src/el_body.cpp \
        $$LH_SRC/src/el_break.cpp \
        $$LH_SRC/src/el_cdata.cpp \
        $$LH_SRC/src/el_comment.cpp \
        $$LH_SRC/src/el_div.cpp \
        $$LH_SRC/src/element.cpp \
        $$LH_SRC/src/el_font.cpp \
        $$LH_SRC/src/el_image.cpp \
        $$LH_SRC/src/el_link.cpp \
        $$LH_SRC/src/el_para.cpp \
        $$LH_SRC/src/el_script.cpp \
        $$LH_SRC/src/el_space.cpp \
        $$LH_SRC/src/el_style.cpp \
        $$LH_SRC/src/el_table.cpp \
        $$LH_SRC/src/el_td.cpp \
        $$LH_SRC/src/el_text.cpp \
        $$LH_SRC/src/el_title.cpp \
        $$LH_SRC/src/el_tr.cpp \
        $$LH_SRC/src/html.cpp \
        $$LH_SRC/src/html_tag.cpp \
        $$LH_SRC/src/iterators.cpp \
        $$LH_SRC/src/media_query.cpp \
        $$LH_SRC/src/style.cpp \
        $$LH_SRC/src/stylesheet.cpp \
        $$LH_SRC/src/table.cpp \
        $$LH_SRC/src/utf8_strings.cpp \
        $$LH_SRC/src/web_color.cpp

    HEADERS += \
        $$LH_SRC/include/litehtml.h \
        $$LH_SRC/src/attributes.h \
        $$LH_SRC/src/background.h \
        $$LH_SRC/src/borders.h \
        $$LH_SRC/src/box.h \
        $$LH_SRC/src/context.h \
        $$LH_SRC/src/css_length.h \
        $$LH_SRC/src/css_margins.h \
        $$LH_SRC/src/css_offsets.h \
        $$LH_SRC/src/css_position.h \
        $$LH_SRC/src/css_selector.h \
        $$LH_SRC/src/document.h \
        $$LH_SRC/src/el_anchor.h \
        $$LH_SRC/src/el_base.h \
        $$LH_SRC/src/el_before_after.h \
        $$LH_SRC/src/el_body.h \
        $$LH_SRC/src/el_break.h \
        $$LH_SRC/src/el_cdata.h \
        $$LH_SRC/src/el_comment.h \
        $$LH_SRC/src/el_div.h \
        $$LH_SRC/src/el_font.h \
        $$LH_SRC/src/el_image.h \
        $$LH_SRC/src/el_link.h \
        $$LH_SRC/src/el_para.h \
        $$LH_SRC/src/el_script.h \
        $$LH_SRC/src/el_space.h \
        $$LH_SRC/src/el_style.h \
        $$LH_SRC/src/el_table.h \
        $$LH_SRC/src/el_td.h \
        $$LH_SRC/src/el_text.h \
        $$LH_SRC/src/el_title.h \
        $$LH_SRC/src/el_tr.h \
        $$LH_SRC/src/element.h \
        $$LH_SRC/src/html.h \
        $$LH_SRC/src/html_tag.h \
        $$LH_SRC/src/iterators.h \
        $$LH_SRC/src/media_query.h \
        $$LH_SRC/src/os_types.h \
        $$LH_SRC/src/style.h \
        $$LH_SRC/src/stylesheet.h \
        $$LH_SRC/src/table.h \
        $$LH_SRC/src/types.h \
        $$LH_SRC/src/utf8_strings.h \
        $$LH_SRC/src/web_color.h

    INCLUDEPATH *= $$LH_SRC/include $$LH_SRC/src $$GB_SRC

    # litehtml without optimization is not fun
    QMAKE_CFLAGS_DEBUG += -O2
    QMAKE_CXXFLAGS_DEBUG += -O2
} else {
    INCLUDEPATH *= $$LITEHTML_INSTALL_DIR/include $$LITEHTML_INSTALL_DIR/include/litehtml
    LITEHTML_LIB_DIR = $$LITEHTML_INSTALL_DIR/lib
    LIBS += -L$$LITEHTML_LIB_DIR -llitehtml -lgumbo

    win32: PRE_TARGETDEPS += $$LITEHTML_LIB_DIR/litehtml.lib $$LITEHTML_LIB_DIR/gumbo.lib
    else:unix: PRE_TARGETDEPS += $$LITEHTML_LIB_DIR/liblitehtml.a $$LITEHTML_LIB_DIR/libgumbo.a
}

HEADERS += \
    $$PWD/container_qpainter.h \
    $$PWD/qlitehtmlwidget.h

SOURCES += \
    $$PWD/container_qpainter.cpp \
    $$PWD/qlitehtmlwidget.cpp

INCLUDEPATH *= $$PWD
win32: DEFINES += LITEHTML_UTF8
