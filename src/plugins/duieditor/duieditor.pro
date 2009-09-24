TEMPLATE = lib
TARGET = DuiEditor
include(../../qtcreatorplugin.pri)
include(duieditor_dependencies.pri)
include(parser/parser.pri)
include(rewriter/rewriter.pri)
DEFINES += DUIEDITOR_LIBRARY \
    QT_CREATOR
INCLUDEPATH += parser \
    rewriter
HEADERS += duieditor.h \
    duieditorfactory.h \
    duieditorplugin.h \
    duihighlighter.h \
    duieditoractionhandler.h \
    duicodecompletion.h \
    duieditorconstants.h \
    duihoverhandler.h \
    duidocument.h \
    duicompletionvisitor.h \
    duimodelmanagerinterface.h \
    duieditor_global.h \
    duimodelmanager.h \
    duicodeformatter.h \
    idcollector.h \
    qmlexpressionundercursor.h \
    qmllookupcontext.h \
    qmlresolveexpression.h \
    qmlsymbol.h \
    qmlfilewizard.h
SOURCES += duieditor.cpp \
    duieditorfactory.cpp \
    duieditorplugin.cpp \
    duihighlighter.cpp \
    duieditoractionhandler.cpp \
    duicodecompletion.cpp \
    duihoverhandler.cpp \
    duidocument.cpp \
    duicompletionvisitor.cpp \
    duimodelmanagerinterface.cpp \
    duimodelmanager.cpp \
    duicodeformatter.cpp \
    idcollector.cpp \
    qmlexpressionundercursor.cpp \
    qmllookupcontext.cpp \
    qmlresolveexpression.cpp \
    qmlsymbol.cpp \
    qmlfilewizard.cpp
RESOURCES += duieditor.qrc
