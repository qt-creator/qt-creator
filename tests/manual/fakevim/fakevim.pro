include(../../auto/qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)
DEFINES += FAKEVIM_STANDALONE

FAKEVIMDIR = $$IDE_SOURCE_TREE/src/plugins/fakevim
UTILSDIR = $$IDE_SOURCE_TREE/src/libs/utils

SOURCES += main.cpp \
            $$FAKEVIMDIR/fakevimhandler.cpp \
            $$FAKEVIMDIR/fakevimactions.cpp \
            $$UTILSDIR/hostosinfo.cpp \
            $$UTILSDIR/qtcassert.cpp

HEADERS += $$FAKEVIMDIR/fakevimhandler.h \
            $$FAKEVIMDIR/fakevimactions.h \
            $$UTILSDIR/hostosinfo.h \
            $$UTILSDIR/qtcassert.h

INCLUDEPATH += $$FAKEVIMDIR $$UTILSDIR
