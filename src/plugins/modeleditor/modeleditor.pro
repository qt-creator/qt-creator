include(../../qtcreatorplugin.pri)

DEFINES += MODELEDITOR_LIBRARY

QT += core gui widgets

!win32:CONFIG(pdf) {
    DEFINES += USE_PDF_CLIPBOARD
}

!win32:CONFIG(svg): {
    QT += svg
    DEFINES += USE_SVG_CLIPBOARD
}

#win32:CONFIG(emf): {
#    DEFINES += USE_EMF USE_EMF_CLIPBOARD
#
#    SOURCES += \
#        emf-engine/qemfpaintengine.cpp \
#        emf-engine/qemfwriter.cpp
#
#    HEADERS += \
#        emf-engine/qemfpaintengine.h \
#        emf-engine/qemfwriter.h
#
#}

SOURCES += \
    actionhandler.cpp \
    classviewcontroller.cpp \
    componentviewcontroller.cpp \
    diagramsviewmanager.cpp \
    dragtool.cpp \
    editordiagramview.cpp \
    elementtasks.cpp \
    extdocumentcontroller.cpp \
    jsextension.cpp \
    modeldocument.cpp \
    modeleditor.cpp \
    modeleditorfactory.cpp \
    modeleditor_file_wizard.cpp \
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
    jsextension.h \
    modeldocument.h \
    modeleditor_constants.h \
    modeleditorfactory.h \
    modeleditor_file_wizard.h \
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
