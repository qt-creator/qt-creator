VPATH += $$PWD
INCLUDEPATH += $$PWD
SOURCES += $$PWD/easinggraph.cpp \
    $$PWD/easingcontextpane.cpp

HEADERS += $$PWD/easinggraph.h \
    $$PWD/easingcontextpane.h

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}
RESOURCES += $$PWD/easingpane.qrc
FORMS += $$PWD/easingcontextpane.ui
