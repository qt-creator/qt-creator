TEMPLATE = lib
TARGET = Valgrind

include(../../qtcreatorplugin.pri)
include(valgrind_dependencies.pri)
include(callgrind/callgrind.pri)
include(memcheck/memcheck.pri)
include(xmlprotocol/xmlprotocol.pri)
QT *= network

CONFIG += exceptions

INCLUDEPATH *= $$PWD

HEADERS += \
    valgrindplugin.h \
    valgrindengine.h \
    valgrindconfigwidget.h \
    valgrindsettings.h \
    valgrindrunner.h \
    valgrindprocess.h \
    callgrindcostdelegate.h \
    callgrindcostview.h \
    callgrindhelper.h \
    callgrindnamedelegate.h \
    callgrindtool.h \
    callgrindvisualisation.h \
    callgrindengine.h \
    workarounds.h \
    callgrindtextmark.h \
    \
    memchecktool.h \
    memcheckengine.h \
    memcheckerrorview.h \
    suppressiondialog.h \
    valgrindtool.h

SOURCES += \
    valgrindplugin.cpp \
    valgrindengine.cpp \
    valgrindconfigwidget.cpp \
    valgrindsettings.cpp \
    valgrindrunner.cpp \
    valgrindprocess.cpp \
    \
    callgrindcostdelegate.cpp \
    callgrindcostview.cpp \
    callgrindhelper.cpp \
    callgrindnamedelegate.cpp \
    callgrindtool.cpp \
    callgrindvisualisation.cpp \
    callgrindengine.cpp \
    workarounds.cpp \
    callgrindtextmark.cpp \
    memchecktool.cpp \
    memcheckengine.cpp \
    memcheckerrorview.cpp \
    suppressiondialog.cpp \
    valgrindtool.cpp

FORMS += \
    valgrindconfigwidget.ui

