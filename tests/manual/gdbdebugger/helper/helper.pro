include(../../../../qtcreator.pri)
TEMPLATE = app

win32:DEFINES += _CRT_SECURE_NO_WARNINGS
SOURCES += $$IDE_SOURCE_TREE/share/qtcreator/gdbmacros/gdbmacros.cpp
SOURCES += main.cpp
