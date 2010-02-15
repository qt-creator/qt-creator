TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)

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
    qmljshighlighter.h \
    qmlhoverhandler.h \
    qmlmodelmanager.h \
    qmlmodelmanagerinterface.h

SOURCES += \
    qmlcodecompletion.cpp \
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmljshighlighter.cpp \
    qmlhoverhandler.cpp \
    qmlmodelmanager.cpp \
    qmlmodelmanagerinterface.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.pluginspec QmlJSEditor.mimetypes.xml
