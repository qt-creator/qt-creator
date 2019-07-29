HEADERS += \
    $$PWD/container_qpainter.h \
    $$PWD/qlitehtmlwidget.h

SOURCES += \
    $$PWD/container_qpainter.cpp \
    $$PWD/qlitehtmlwidget.cpp

INCLUDEPATH += $$PWD $$LITEHTML_INSTALL_DIR/include $$LITEHTML_INSTALL_DIR/include/litehtml
LIBS += -L$$LITEHTML_INSTALL_DIR/lib -llitehtml -lgumbo

win32: PRE_TARGET_DEPS += $$LITEHTML_INSTALL_DIR/lib/litehtml.lib $$LITEHTML_INSTALL_DIR/lib/gumbo.lib
else:unix: PRE_TARGET_DEPS += $$LITEHTML_INSTALL_DIR/lib/liblitehtml.a $$LITEHTML_INSTALL_DIR/lib/libgumbo.a
