FAKEVIMHOME = ../../../src/plugins/fakevim
UTILSDIR = ../../../src/libs

DEFINES += QTCREATOR_UTILS_STATIC_LIB

include(../../../src/libs/utils/utils-lib.pri)

SOURCES += \
	main.cpp \
	$$FAKEVIMHOME/fakevimhandler.cpp \
	$$FAKEVIMHOME/fakevimactions.cpp \
        $$FAKEVIMHOME/fakevimsyntax.cpp

HEADERS += \
	$$FAKEVIMHOME/fakevimhandler.h \
	$$FAKEVIMHOME/fakevimactions.h \
        $$FAKEVIMHOME/fakevimsyntax.h

INCLUDEPATH += $$FAKEVIMHOME $$UTILSDIR

