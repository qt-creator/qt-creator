TEMPLATE = lib
TARGET = DuiEditor
QT += script

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(../../shared/indenter/indenter.pri)

include($$(QTDIR_DUI)/src/declarative/qml/parser/parser.pri)
INCLUDEPATH += $$(QTDIR_DUI)/src/declarative/qml    # FIXME: remove me

include(rewriter/rewriter.pri)

HEADERS += duieditor.h \
duieditorfactory.h \
duieditorplugin.h \
duihighlighter.h \
duieditoractionhandler.h \
duicodecompletion.h \
duieditorconstants.h

SOURCES += duieditor.cpp \
duieditorfactory.cpp \
duieditorplugin.cpp \
duihighlighter.cpp \
duieditoractionhandler.cpp \
duicodecompletion.cpp

RESOURCES += duieditor.qrc
