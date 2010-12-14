include(../../../qtcreator.pri)
include(../../../src/libs/utils/utils.pri)

FAKEVIMDIR = $$IDE_SOURCE_TREE/src/plugins/fakevim
LIBSDIR = $$IDE_SOURCE_TREE/src/libs

SOURCES += main.cpp \
            $$FAKEVIMDIR/fakevimhandler.cpp \
            $$FAKEVIMDIR/fakevimactions.cpp \
            $$FAKEVIMDIR/fakevimsyntax.cpp \

HEADERS += $$FAKEVIMDIR/fakevimhandler.h \
            $$FAKEVIMDIR/fakevimactions.h \
            $$FAKEVIMDIR/fakevimsyntax.h \

INCLUDEPATH += $$FAKEVIMDIR $$LIBSDIR

