TEMPLATE = lib
TARGET = DuiEditor

include(../../qtcreatorplugin.pri)
include(duieditor_dependencies.pri)

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
duidocument.h \
duicompletionvisitor.h

SOURCES += duieditor.cpp \
duieditorfactory.cpp \
duieditorplugin.cpp \
duihighlighter.cpp \
duieditoractionhandler.cpp \
duicodecompletion.cpp \
duihoverhandler.cpp \
duidocument.cpp \
duicompletionvisitor.cpp

RESOURCES += duieditor.qrc
