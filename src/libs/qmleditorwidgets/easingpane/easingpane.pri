VPATH += $$PWD
INCLUDEPATH += $$PWD
SOURCES += $$PWD/easinggraph.cpp \
    $$PWD/easingcontextpane.cpp

HEADERS += $$PWD/easinggraph.h \
    $$PWD/easingcontextpane.h

QT += declarative

RESOURCES += $$PWD/easingpane.qrc
FORMS += $$PWD/easingcontextpane.ui
