include(../../auto/qttest.pri)
include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

FAKEVIMDIR = $$IDE_SOURCE_TREE/src/plugins/fakevim
LIBSDIR = $$IDE_SOURCE_TREE/src/libs

SOURCES += main.cpp \
            $$FAKEVIMDIR/fakevimhandler.cpp \
            $$FAKEVIMDIR/fakevimactions.cpp

HEADERS += $$FAKEVIMDIR/fakevimhandler.h \
            $$FAKEVIMDIR/fakevimactions.h

INCLUDEPATH += $$FAKEVIMDIR $$LIBSDIR

