include(../../qtcreatortool.pri)

TARGET = qtcreator_processlauncher
CONFIG += console c++17
CONFIG -= app_bundle
QT = core network

UTILS_DIR = $$PWD/../../libs/utils

INCLUDEPATH += $$UTILS_DIR

HEADERS += \
    launcherlogging.h \
    launchersockethandler.h \
    $$UTILS_DIR/launcherpackets.h

SOURCES += \
    launcherlogging.cpp \
    launchersockethandler.cpp \
    processlauncher-main.cpp \
    $$UTILS_DIR/launcherpackets.cpp
