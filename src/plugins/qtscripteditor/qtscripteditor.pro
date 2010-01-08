TEMPLATE = lib
TARGET = QtScriptEditor
QT += script

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(parser/parser.pri)

DEPENDPATH += ../../shared/qscripthighlighter

HEADERS += qtscripteditor.h \
qtscripteditorfactory.h \
qtscripteditorplugin.h \
qtscripthighlighter.h \
qtscriptcodecompletion.h

SOURCES += qtscripteditor.cpp \
qtscripteditorfactory.cpp \
qtscripteditorplugin.cpp \
qtscripthighlighter.cpp \
qtscriptcodecompletion.cpp

RESOURCES += qtscripteditor.qrc
