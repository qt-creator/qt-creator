TEMPLATE = lib
TARGET = QmlJSEditor
include(../../qtcreatorplugin.pri)
include(qmljseditor_dependencies.pri)

CONFIG += help
DEFINES += \
    QMLJSEDITOR_LIBRARY \
    QT_CREATOR

HEADERS += \
    qmljscodecompletion.h \
    qmljseditor.h \
    qmljseditor_global.h \
    qmljseditoractionhandler.h \
    qmljseditorconstants.h \
    qmljseditorfactory.h \
    qmljseditorplugin.h \
    qmlexpressionundercursor.h \
    qmlfilewizard.h \
    qmljshighlighter.h \
    qmljshoverhandler.h \
    qmlmodelmanager.h \
    qmlmodelmanagerinterface.h

SOURCES += \
    qmljscodecompletion.cpp \
    qmljseditor.cpp \
    qmljseditoractionhandler.cpp \
    qmljseditorfactory.cpp \
    qmljseditorplugin.cpp \
    qmlexpressionundercursor.cpp \
    qmlfilewizard.cpp \
    qmljshighlighter.cpp \
    qmljshoverhandler.cpp \
    qmlmodelmanager.cpp \
    qmlmodelmanagerinterface.cpp

RESOURCES += qmljseditor.qrc
OTHER_FILES += QmlJSEditor.pluginspec QmlJSEditor.mimetypes.xml
