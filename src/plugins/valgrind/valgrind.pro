TEMPLATE = lib
TARGET = Valgrind

include(../../qtcreatorplugin.pri)
include(valgrind_dependencies.pri)
include(valgrind/valgrind.pri)
QT *= network

INCLUDEPATH *= $$PWD

HEADERS += \
    valgrindplugin.h \
    valgrindengine.h \
    valgrindconfigwidget.h \
    valgrindsettings.h \
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
    suppressiondialog.h

SOURCES += \
    valgrindplugin.cpp \
    valgrindengine.cpp \
    valgrindconfigwidget.cpp \
    valgrindsettings.cpp \
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
    suppressiondialog.cpp

FORMS += \
    valgrindconfigwidget.ui \
    suppressiondialog.ui

