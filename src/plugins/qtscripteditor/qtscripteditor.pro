TEMPLATE = lib
TARGET = QtScriptEditor
QT += script

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(../../shared/indenter/indenter.pri)

HEADERS += qtscripteditor.h \
qtscripteditorfactory.h \
qtscripteditorplugin.h \
qtscripthighlighter.h \
qtscripteditoractionhandler.h

SOURCES += qtscripteditor.cpp \
qtscripteditorfactory.cpp \
qtscripteditorplugin.cpp \
qtscripthighlighter.cpp \
qtscripteditoractionhandler.cpp
RESOURCES += qtscripteditor.qrc
