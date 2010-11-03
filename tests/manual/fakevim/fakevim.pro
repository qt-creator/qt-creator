FAKEVIMHOME = ../../../src/plugins/fakevim
LIBSDIR = ../../../src/libs

SOURCES += \
	main.cpp \
	$$FAKEVIMHOME/fakevimhandler.cpp \
	$$FAKEVIMHOME/fakevimactions.cpp \
        $$FAKEVIMHOME/fakevimsyntax.cpp \
        $$LIBSDIR/utils/basevalidatinglineedit.cpp \
        $$LIBSDIR/utils/environment.cpp \
        $$LIBSDIR/utils/pathchooser.cpp \
        $$LIBSDIR/utils/savedaction.cpp

HEADERS += \
	$$FAKEVIMHOME/fakevimhandler.h \
	$$FAKEVIMHOME/fakevimactions.h \
        $$FAKEVIMHOME/fakevimsyntax.h \
        $$LIBSDIR/utils/basevalidatinglineedit.h \
        $$LIBSDIR/utils/environment.h \
        $$LIBSDIR/utils/pathchooser.h \
        $$LIBSDIR/utils/savedaction.h

INCLUDEPATH += $$FAKEVIMHOME $$LIBSDIR

