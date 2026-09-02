include(../gtest_dependency.pri)

TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

INCLUDEPATH += $$PWD/../gtlib
LIBS += -L$$OUT_PWD/../gtlib -lgtlibtests

SOURCES += main.cpp
