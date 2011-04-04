TEMPLATE = app
TARGET = modeltest

macx:CONFIG -= app_bundle

!win32 {
    include(../../../qtcreator.pri)
    include(../../../src/plugins/callgrind/callgrind.pri)
}

SRCDIR = $$IDE_SOURCE_TREE/src

SOURCES += \
    modeltest.cpp \
    $$SRCDIR/plugins/callgrind/callgrindcostdelegate.cpp \
    $$SRCDIR/plugins/callgrind/callgrindhelper.cpp \
    $$SRCDIR/plugins/callgrind/callgrindcostview.cpp \
    $$SRCDIR/plugins/callgrind/callgrindnamedelegate.cpp \
    $$SRCDIR/plugins/callgrind/callgrindwidgethandler.cpp \
    $$SRCDIR/plugins/callgrind/callgrindvisualisation.cpp \

HEADERS += \
    modeltest.h \
    $$SRCDIR/plugins/callgrind/callgrindcostdelegate.h \
    $$SRCDIR/plugins/callgrind/callgrindcostview.h \
    $$SRCDIR/plugins/callgrind/callgrindhelper.h \
    $$SRCDIR/plugins/callgrind/callgrindnamedelegate.h \
    $$SRCDIR/plugins/callgrind/callgrindwidgethandler.h \
    $$SRCDIR/plugins/callgrind/callgrindvisualisation.h \

LIBS += -L$$IDE_PLUGIN_PATH/Nokia

INCLUDEPATH *= $$IDE_BUILD_TREE/src/plugins/coreplugin # for ide_version.h

DEFINES += "DISABLE_CALLGRIND_WORKAROUNDS"
