TEMPLATE = lib
TARGET = Valgrind

include(../../qtcreatorplugin.pri)
include(valgrind_dependencies.pri)
include(callgrind/callgrind.pri)
include(memcheck/memcheck.pri)
include(xmlprotocol/xmlprotocol.pri)
QT *= network

INCLUDEPATH *= $$PWD

HEADERS += \
    valgrindplugin.h \
    valgrindengine.h \
    valgrindconfigwidget.h \
    valgrindsettings.h \
    valgrindrunner.h \
    valgrindprocess.h \
    \
    callgrindconfigwidget.h \
    callgrindcostdelegate.h \
    callgrindcostview.h \
    callgrindhelper.h \
    callgrindnamedelegate.h \
    callgrindsettings.h \
    callgrindtool.h \
    callgrindvisualisation.h \
    callgrindengine.h \
    workarounds.h \
    callgrindtextmark.h \
    \
    memchecktool.h \
    memcheckengine.h \
    memcheckerrorview.h \
    memchecksettings.h \
    memcheckconfigwidget.h \
    suppressiondialog.h

SOURCES += \
    valgrindplugin.cpp \
    valgrindengine.cpp \
    valgrindconfigwidget.cpp \
    valgrindsettings.cpp \
    valgrindrunner.cpp \
    valgrindprocess.cpp \
    \
    callgrindconfigwidget.cpp \
    callgrindcostdelegate.cpp \
    callgrindcostview.cpp \
    callgrindhelper.cpp \
    callgrindnamedelegate.cpp \
    callgrindsettings.cpp \
    callgrindtool.cpp \
    callgrindvisualisation.cpp \
    callgrindengine.cpp \
    workarounds.cpp \
    callgrindtextmark.cpp \
    memchecktool.cpp \
    memcheckengine.cpp \
    memcheckerrorview.cpp \
    memchecksettings.cpp \
    memcheckconfigwidget.cpp \
    suppressiondialog.cpp

FORMS += \
    valgrindconfigwidget.ui \
    callgrindconfigwidget.ui \
    suppressiondialog.ui \
    memcheckconfigwidget.ui

