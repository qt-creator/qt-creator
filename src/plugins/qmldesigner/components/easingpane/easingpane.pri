VPATH += $$PWD
INCLUDEPATH += $$PWD
#include($$PWD/qtgradienteditor/qtgradienteditor.pri)
SOURCES += easinggraph.cpp \
    easingcontextpane.cpp

HEADERS += easinggraph.h \
    easingcontextpane.h

QT += declarative
RESOURCES += easingpane.qrc
FORMS += components/easingpane/easingcontextpane.ui
