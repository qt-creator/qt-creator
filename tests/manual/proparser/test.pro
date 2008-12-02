#comment

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += console
QT += xml

win32 {
    PROPARSERPATH = ../../some/other/test/test
    unix:PROPARSERPATH = ../../test/test
}

# test comment
PROPARSERPATH = ../../../src/plugins/qt4projectmanager/proparser
INCLUDEPATH += $$PROPARSERPATH

# Input
HEADERS += $$PROPARSERPATH/proitems.h \ # test comment2
	$$PROPARSERPATH/proxml.h \
	$$PROPARSERPATH/proreader.h
SOURCES += main.cpp \
	$$PROPARSERPATH/proitems.cpp \
# test comment 3
	$$PROPARSERPATH/proxml.cpp \
	$$PROPARSERPATH/proreader.cpp
