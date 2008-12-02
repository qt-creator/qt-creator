TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += console
QT += xml

PROXMLPATH = ../../../src/plugins/qt4projectmanager/proparser 
PROPARSERPATH = $$(QTDIR)/tools/linguist/shared
INCLUDEPATH += $$PROPARSERPATH $$PROXMLPATH

# Input
HEADERS += $$PROPARSERPATH/proitems.h \
	$$PROXMLPATH/proxml.h \
	$$PROPARSERPATH/proreader.h
SOURCES += main.cpp \
	$$PROPARSERPATH/proitems.cpp \
	$$PROXMLPATH/proxml.cpp \
	$$PROPARSERPATH/proreader.cpp

