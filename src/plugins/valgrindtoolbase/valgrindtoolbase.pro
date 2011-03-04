TEMPLATE = lib
TARGET = ValgrindToolBase

DEFINES += VALGRINDTOOLBASE_LIBRARY

include(../../qtcreatorplugin.pri)
include(valgrindtoolbase_dependencies.pri)

QT += network

# Valgrind Tool Base files

HEADERS += \
    valgrindtoolbaseplugin.h \
    valgrindtoolbase_global.h \
    valgrindengine.h \
    valgrindconfigwidget.h \
    valgrindsettings.h

SOURCES += \
    valgrindtoolbaseplugin.cpp \
    valgrindengine.cpp \
    valgrindconfigwidget.cpp \
    valgrindsettings.cpp

FORMS += \
    valgrindconfigwidget.ui
