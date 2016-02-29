include(../../qtcreatorplugin.pri)

DEFINES += MODELEDITOR_LIBRARY

QT += core gui widgets

isEmpty(QT.svg.name): DEFINES += QT_NO_SVG

SOURCES += \
    actionhandler.cpp \
    classviewcontroller.cpp \
    componentviewcontroller.cpp \
    diagramsviewmanager.cpp \
    dragtool.cpp \
    editordiagramview.cpp \
    elementtasks.cpp \
    extdocumentcontroller.cpp \
    extpropertiesmview.cpp \
    jsextension.cpp \
    modeldocument.cpp \
    modeleditor.cpp \
    modeleditorfactory.cpp \
    modeleditor_plugin.cpp \
    modelindexer.cpp \
    modelsmanager.cpp \
    openelementvisitor.cpp \
    pxnodecontroller.cpp \
    pxnodeutilities.cpp \
    settingscontroller.cpp \
    uicontroller.cpp

HEADERS += \
    actionhandler.h \
    classviewcontroller.h \
    componentviewcontroller.h \
    diagramsviewmanager.h \
    dragtool.h \
    editordiagramview.h \
    elementtasks.h \
    extdocumentcontroller.h \
    extpropertiesmview.h \
    jsextension.h \
    modeldocument.h \
    modeleditor_constants.h \
    modeleditorfactory.h \
    modeleditor_global.h \
    modeleditor.h \
    modeleditor_plugin.h \
    modelindexer.h \
    modelsmanager.h \
    openelementvisitor.h \
    pxnodecontroller.h \
    pxnodeutilities.h \
    settingscontroller.h \
    uicontroller.h

OTHER_FILES += \
    ModelEditor.json.in

RESOURCES += \
    resources/modeleditor.qrc

FORMS +=
