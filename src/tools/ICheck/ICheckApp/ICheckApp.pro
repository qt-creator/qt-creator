#-------------------------------------------------
#
# Project created by QtCreator 2010-01-18T15:27:35
#
#-------------------------------------------------

QT       -= gui

TARGET = ICheckApp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../ICheckLib
CONFIG(debug, debug|release){
    LIBS += -L../ICheckLib/debug -lICheckLibd
}
else {
    LIBS += -L../ICheckLib/release -lICheckLib
}

SOURCES += main.cpp
