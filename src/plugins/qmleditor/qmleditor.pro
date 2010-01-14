TEMPLATE = lib
TARGET = QmlEditor
include(../../qtcreatorplugin.pri)
include(qmleditor_dependencies.pri)

include(../../shared/qscripthighlighter/qscripthighlighter.pri)
DEPENDPATH += ../../shared/qscripthighlighter


CONFIG += help
DEFINES += \
    QMLEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmlcodecompletion.h \
    qmleditor.h \
    qmleditor_global.h \
    qmleditoractionhandler.h \
    qmleditorconstants.h \
    qmleditorfactory.h \
    qmleditorplugin.h \
    qmlexpressionundercursor.h \
    qmlfilewizard.h \
    qmlhighlighter.h \
    qmlhoverhandler.h \
    qmllookupcontext.h \
    qmlmodelmanager.h \
    qmlmodelmanagerinterface.h \
    qmlresolveexpression.h

SOURCES += \
    qmlcodecompletion.cpp \
    qmleditor.cpp \
    qmleditoractionhandler.cpp \
    qmleditorfactory.cpp \
    qmleditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmlhighlighter.cpp \
    qmlhoverhandler.cpp \
    qmllookupcontext.cpp \
    qmlmodelmanager.cpp \
    qmlmodelmanagerinterface.cpp \
    qmlresolveexpression.cpp

RESOURCES += qmleditor.qrc
OTHER_FILES += QmlEditor.pluginspec
