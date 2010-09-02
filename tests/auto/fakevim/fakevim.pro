CONFIG += qtestlib testcase

# Defines import symbol as empty
DEFINES+=QTCREATOR_UTILS_STATIC_LIB

FAKEVIMDIR = ../../../src/plugins/fakevim
UTILSDIR = ../../../src/libs

SOURCES += \
	$$FAKEVIMDIR/fakevimhandler.cpp \
	$$FAKEVIMDIR/fakevimactions.cpp \
	$$FAKEVIMDIR/fakevimsyntax.cpp \
	$$UTILSDIR/utils/savedaction.cpp \
	$$UTILSDIR/utils/pathchooser.cpp \
	$$UTILSDIR/utils/basevalidatinglineedit.cpp \
        tst_fakevim.cpp

HEADERS += \
	$$FAKEVIMDIR/fakevimhandler.h \
	$$FAKEVIMDIR/fakevimactions.h \
	$$FAKEVIMDIR/fakevimsyntax.h \
	$$UTILSDIR/utils/savedaction.h \
	$$UTILSDIR/utils/pathchooser.h \
	$$UTILSDIR/utils/basevalidatinglineedit.h \

INCLUDEPATH += $$FAKEVIMDIR $$UTILSDIR
