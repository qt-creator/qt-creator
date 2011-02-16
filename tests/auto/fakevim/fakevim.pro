include(../qttest.pri)

include($$IDE_SOURCE_TREE/src/libs/utils/utils.pri)

FAKEVIMDIR = $$IDE_SOURCE_TREE/src/plugins/fakevim
UTILSDIR = $$IDE_SOURCE_TREE/src/libs

SOURCES += \
        $$FAKEVIMDIR/fakevimhandler.cpp \
        $$FAKEVIMDIR/fakevimactions.cpp \
        tst_fakevim.cpp

HEADERS += \
        $$FAKEVIMDIR/fakevimhandler.h \
        $$FAKEVIMDIR/fakevimactions.h

INCLUDEPATH += $$FAKEVIMDIR $$UTILSDIR
