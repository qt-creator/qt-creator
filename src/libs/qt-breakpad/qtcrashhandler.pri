isEmpty(BREAKPAD_SOURCE_DIR): return()

QT += network

INCLUDEPATH += \
    $$BREAKPAD_SOURCE_DIR/src \
    $$PWD/qtcrashhandler

SOURCES += \
    $$PWD/qtcrashhandler/main.cpp \
    $$PWD/qtcrashhandler/mainwidget.cpp \
    $$PWD/qtcrashhandler/detaildialog.cpp \
    $$PWD/qtcrashhandler/dumpsender.cpp

HEADERS += \
    $$PWD/qtcrashhandler/mainwidget.h \
    $$PWD/qtcrashhandler/detaildialog.h \
    $$PWD/qtcrashhandler/dumpsender.h

FORMS += \
    $$PWD/qtcrashhandler/mainwidget.ui
