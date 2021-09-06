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
    $$UTILS_DIR/launcherpackets.h \
    $$UTILS_DIR/processreaper.h \
    $$UTILS_DIR/processutils.h \
    $$UTILS_DIR/qtcassert.h

SOURCES += \
    launcherlogging.cpp \
    launchersockethandler.cpp \
    processlauncher-main.cpp \
    $$UTILS_DIR/launcherpackets.cpp \
    $$UTILS_DIR/processreaper.cpp \
    $$UTILS_DIR/processutils.cpp \
    $$UTILS_DIR/qtcassert.cpp
