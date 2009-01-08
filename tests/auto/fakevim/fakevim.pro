
QT += testlib

FAKEVIMSOURCE = /data/qt-creator/src/plugins/fakevim

INCLUDEPATH += $$FAKEVIMSOURCE

SOURCES += \
	main.cpp \
	$$FAKEVIMSOURCE/handler.cpp

HEADERS += \
	$$FAKEVIMSOURCE/handler.h

