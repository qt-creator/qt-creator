include(../../../qtcreator.pri)
include(../../auto/qttestrpath.pri)
include($$IDE_SOURCE_TREE/src/plugins/valgrind/valgrind_test.pri)
include($$IDE_SOURCE_TREE/src/libs/languageutils/languageutils.pri)

TEMPLATE = app
TARGET = modeltest

macx:CONFIG -= app_bundle


SRCDIR = $$IDE_SOURCE_TREE/src

SOURCES += \
    modeltest.cpp \
    $$SRCDIR/plugins/valgrind/callgrindcostdelegate.cpp \
    $$SRCDIR/plugins/valgrind/callgrindhelper.cpp \
    $$SRCDIR/plugins/valgrind/callgrindcostview.cpp \
    $$SRCDIR/plugins/valgrind/callgrindnamedelegate.cpp \
    $$SRCDIR/plugins/valgrind/callgrindwidgethandler.cpp \
    $$SRCDIR/plugins/valgrind/callgrindvisualisation.cpp \

HEADERS += \
    modeltest.h \
    $$SRCDIR/plugins/valgrind/callgrindcostdelegate.h \
    $$SRCDIR/plugins/valgrind/callgrindcostview.h \
    $$SRCDIR/plugins/valgrind/callgrindhelper.h \
    $$SRCDIR/plugins/valgrind/callgrindnamedelegate.h \
    $$SRCDIR/plugins/valgrind/callgrindwidgethandler.h \
    $$SRCDIR/plugins/valgrind/callgrindvisualisation.h \

LIBS *= -L$$IDE_PLUGIN_PATH

INCLUDEPATH *= $$IDE_BUILD_TREE/src/plugins/coreplugin # for ide_version.h

DEFINES += "DISABLE_CALLGRIND_WORKAROUNDS"
