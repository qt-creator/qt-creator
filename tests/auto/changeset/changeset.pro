
QT += testlib

# Defines import symbol as empty
DEFINES+=QTCREATOR_UTILS_STATIC_LIB

UTILSDIR = ../../../src/libs

SOURCES += \
	tst_changeset.cpp \
	$$UTILSDIR/utils/changeset.cpp

HEADERS += \
	$$UTILSDIR/utils/changeset.h

INCLUDEPATH += $$UTILSDIR

TARGET=tst_$$TARGET
