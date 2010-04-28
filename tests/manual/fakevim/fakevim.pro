
FAKEVIMHOME = ../../../src/plugins/fakevim
UTILSDIR = ../../../src/libs

SOURCES += \
	main.cpp \
	$$FAKEVIMHOME/fakevimhandler.cpp \
	$$FAKEVIMHOME/fakevimactions.cpp \
	$$FAKEVIMHOME/fakevimsyntax.cpp \
	$$UTILSDIR/utils/savedaction.cpp \
	$$UTILSDIR/utils/pathchooser.cpp \
	$$UTILSDIR/utils/basevalidatinglineedit.cpp \

HEADERS += \
	$$FAKEVIMHOME/fakevimhandler.h \
	$$FAKEVIMHOME/fakevimactions.h \
	$$FAKEVIMHOME/fakevimsyntax.h \
	$$UTILSDIR/utils/savedaction.h \
	$$UTILSDIR/utils/pathchooser.h \
	$$UTILSDIR/utils/basevalidatinglineedit.h \

INCLUDEPATH += $$FAKEVIMHOME $$UTILSDIR

