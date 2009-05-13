TEMPLATE = lib
TARGET = DuiEditor
QT += script

include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(../../shared/indenter/indenter.pri)

DUI=$$(QTDIR_DUI)
isEmpty(DUI):DUI=$$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)

!isEmpty(DUI):exists($$DUI/src/declarative/qml/parser) {
    include($$DUI/src/declarative/qml/parser/parser.pri)
    INCLUDEPATH += $$DUI/src/declarative/qml
} else {
    error(run with export QTDIR_DUI=<path to kinetic/qt>)
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
