TEMPLATE = lib
TARGET = Designer
DEFINES += DESIGNER_LIBRARY

include(../../qtcreatorplugin.pri)
include(../../shared/designerintegrationv2/designerintegration.pri)
include(cpp/cpp.pri)
include(designer_dependencies.pri)

# -- figure out shared dir location
!exists($$[QT_INSTALL_HEADERS]/QtDesigner/private/qdesigner_integration_p.h) {
    QT_SOURCE_TREE=$$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)
    INCLUDEPATH += $$QT_SOURCE_TREE/include
}

INCLUDEPATH += $$QMAKE_INCDIR_QT/QtDesigner \
    ../../tools/utils

qtAddLibrary(QtDesigner)
qtAddLibrary(QtDesignerComponents)

QT+=xml

HEADERS += formeditorplugin.h \
        formeditorfactory.h \
        formwindoweditor.h \
        formwindowfile.h \
        formwizard.h \
        qtcreatorintegration.h \
        designerconstants.h \
        settingspage.h \
        editorwidget.h \
        formeditorw.h \
        settingsmanager.h \
        formtemplatewizardpage.h \
        formwizarddialog.h \
        codemodelhelpers.h \
        designer_export.h \
    designerxmleditor.h \
    designercontext.h \
    faketoolbar.h \
    formeditorstack.h \
    editordata.h \
    resourcehandler.h

SOURCES += formeditorplugin.cpp \
        formeditorfactory.cpp \
        formwindoweditor.cpp \
        formwindowfile.cpp \
        formwizard.cpp \
        qtcreatorintegration.cpp \
        settingspage.cpp \
        editorwidget.cpp \
        formeditorw.cpp \
        settingsmanager.cpp \
        formtemplatewizardpage.cpp \
        formwizarddialog.cpp \
        codemodelhelpers.cpp \
    designerxmleditor.cpp \
    designercontext.cpp \
    faketoolbar.cpp \
    formeditorstack.cpp \
    resourcehandler.cpp

RESOURCES += designer.qrc

OTHER_FILES += Designer.pluginspec Designer.mimetypes.xml
