TEMPLATE = lib
TARGET = Callgrind

DEFINES += CALLGRIND_LIBRARY

include(../../qtcreatorplugin.pri)
include(callgrind_dependencies.pri)

# Callgrind files

HEADERS += \
    callgrindplugin.h \
    callgrind_global.h \
    callgrindconfigwidget.h \
    callgrindcostdelegate.h \
    callgrindcostview.h \
    callgrindhelper.h \
    callgrindnamedelegate.h \
    callgrindsettings.h \
    callgrindtool.h \
    callgrindvisualisation.h \
    callgrindwidgethandler.h \
    callgrindengine.h \
    workarounds.h \
    callgrindtextmark.h

SOURCES += \
    callgrindplugin.cpp \
    callgrindconfigwidget.cpp \
    callgrindcostdelegate.cpp \
    callgrindcostview.cpp \
    callgrindhelper.cpp \
    callgrindnamedelegate.cpp \
    callgrindsettings.cpp \
    callgrindtool.cpp \
    callgrindvisualisation.cpp \
    callgrindwidgethandler.cpp \
    callgrindengine.cpp \
    workarounds.cpp \
    callgrindtextmark.cpp

FORMS += \
    callgrindconfigwidget.ui
