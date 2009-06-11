TEMPLATE = lib
TARGET = DuiEditor

include(../../qtcreatorplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../shared/qscripthighlighter/qscripthighlighter.pri)
include(../../shared/indenter/indenter.pri)

include(parser/parser.pri)
include(rewriter/rewriter.pri)

INCLUDEPATH += parser rewriter

HEADERS += duieditor.h \
duieditorfactory.h \
duieditorplugin.h \
duihighlighter.h \
duieditoractionhandler.h \
duicodecompletion.h \
duieditorconstants.h \
duihoverhandler.h \
duidocument.h

SOURCES += duieditor.cpp \
duieditorfactory.cpp \
duieditorplugin.cpp \
duihighlighter.cpp \
duieditoractionhandler.cpp \
duicodecompletion.cpp \
duihoverhandler.cpp \
duidocument.cpp

RESOURCES += duieditor.qrc
