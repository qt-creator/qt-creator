TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)

include(../../shared/qscripthighlighter/qscripthighlighter.pri)
DEPENDPATH += ../../shared/qscripthighlighter


CONFIG += help
DEFINES += \
    QMLJSEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmlcodecompletion.h \
    qmljseditor.h \
    qmljseditor_global.h \
    qmljseditoractionhandler.h \
    qmljseditorconstants.h \
    qmljseditorfactory.h \
    qmljseditorplugin.h \
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
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmlhighlighter.cpp \
    qmlhoverhandler.cpp \
    qmllookupcontext.cpp \
    qmlmodelmanager.cpp \
    qmlmodelmanagerinterface.cpp \
    qmlresolveexpression.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.pluginspec QmlJSEditor.mimetypes.xml
