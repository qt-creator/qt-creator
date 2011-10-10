include(../../../../qtcreator.pri)
TEMPLATE = app
greaterThan(QT_MAJOR_VERSION, 4):QT *= widgets

win32:DEFINES += _CRT_SECURE_NO_WARNINGS
SOURCES += $$IDE_SOURCE_TREE/share/qtcreator/dumper/dumper.cpp
SOURCES += main.cpp
