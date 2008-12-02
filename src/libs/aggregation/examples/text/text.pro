TARGET = text
TEMPLATE = app
QT += core \
    gui
DEFINES += AGGREGATION_LIBRARY
INCLUDEPATH += ../../
SOURCES += main.cpp \
    ../../aggregate.cpp
HEADERS += main.h \
    myinterfaces.h \
    ../../aggregate.h \
    ../../aggregation_global.h
FORMS += main.ui

