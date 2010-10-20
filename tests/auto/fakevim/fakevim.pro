CONFIG += qtestlib testcase

# Defines import symbol as empty
DEFINES+=QTCREATOR_UTILS_STATIC_LIB

include(../../../src/libs/utils/utils-lib.pri)

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
