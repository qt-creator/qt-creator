include(../qttest.pri)

# Defines import symbol as empty
DEFINES+=QTCREATOR_UTILS_STATIC_LIB

include(../../../src/libs/utils/utils.pri)

FAKEVIMDIR = ../../../src/plugins/fakevim
UTILSDIR = ../../../src/libs

SOURCES += \
        $$FAKEVIMDIR/fakevimhandler.cpp \
        $$FAKEVIMDIR/fakevimactions.cpp \
        $$FAKEVIMDIR/fakevimsyntax.cpp \
        tst_fakevim.cpp

HEADERS += \
        $$FAKEVIMDIR/fakevimhandler.h \
        $$FAKEVIMDIR/fakevimactions.h \
        $$FAKEVIMDIR/fakevimsyntax.h \

INCLUDEPATH += $$FAKEVIMDIR $$UTILSDIR
