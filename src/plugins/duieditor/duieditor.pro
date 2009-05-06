TEMPLATE = lib
TARGET = DuiEditor
QT += script

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(../../shared/indenter/indenter.pri)

QT_BUILD_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_BUILD_TREE)
QT_SOURCE_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)

exists($$QT_SOURCE_TREE/src/declarative/qml/parser) {
    include($$QT_SOURCE_TREE/src/declarative/qml/parser/parser.pri)
    INCLUDEPATH += $$QT_SOURCE_TREE/src/declarative/qml
} else {
    DUI=$$(QTDIR_DUI)
    isEmpty(DUI):error(run with export QTDIR_DUI=<path to kinetic/qt>)
    include($$(QTDIR_DUI)/src/declarative/qml/parser/parser.pri)
    INCLUDEPATH += $$(QTDIR_DUI)/src/declarative/qml
}


include(rewriter/rewriter.pri)

HEADERS += duieditor.h \
duieditorfactory.h \
duieditorplugin.h \
duihighlighter.h \
duieditoractionhandler.h \
duicodecompletion.h \
duieditorconstants.h \
duihoverhandler.h

SOURCES += duieditor.cpp \
duieditorfactory.cpp \
duieditorplugin.cpp \
duihighlighter.cpp \
duieditoractionhandler.cpp \
duicodecompletion.cpp \
duihoverhandler.cpp

RESOURCES += duieditor.qrc
