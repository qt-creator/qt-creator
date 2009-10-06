TEMPLATE = lib
TARGET = QmlEditor
include(../../qtcreatorplugin.pri)
include(qmleditor_dependencies.pri)
include(parser/parser.pri)
include(rewriter/rewriter.pri)
DEFINES += QMLEDITOR_LIBRARY \
    QT_CREATOR
INCLUDEPATH += parser \
    rewriter
HEADERS += qmleditor.h \
    qmleditorfactory.h \
    qmleditorplugin.h \
    qmlhighlighter.h \
    qmleditoractionhandler.h \
    qmlcodecompletion.h \
    qmleditorconstants.h \
    qmlhoverhandler.h \
    qmldocument.h \
    qmlmodelmanagerinterface.h \
    qmleditor_global.h \
    qmlmodelmanager.h \
    qmlcodeformatter.h \
    idcollector.h \
    qmlexpressionundercursor.h \
    qmllookupcontext.h \
    qmlresolveexpression.h \
    qmlsymbol.h \
    qmlfilewizard.h
SOURCES += qmleditor.cpp \
    qmleditorfactory.cpp \
    qmleditorplugin.cpp \
    qmlhighlighter.cpp \
    qmleditoractionhandler.cpp \
    qmlcodecompletion.cpp \
    qmlhoverhandler.cpp \
    qmldocument.cpp \
    qmlmodelmanagerinterface.cpp \
    qmlmodelmanager.cpp \
    qmlcodeformatter.cpp \
    idcollector.cpp \
    qmlexpressionundercursor.cpp \
    qmllookupcontext.cpp \
    qmlresolveexpression.cpp \
    qmlsymbol.cpp \
    qmlfilewizard.cpp
RESOURCES += qmleditor.qrc
