include(../../../qtcreator.pri)
include(../../auto/qttestrpath.pri)
include($$IDE_SOURCE_TREE/src/plugins/callgrind/callgrind.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils.pri)

TEMPLATE = app
TARGET = modeltest

macx:CONFIG -= app_bundle


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
