TEMPLATE = lib
TARGET = Designer

include(../../qworkbenchplugin.pri)
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
        formwindowhost.h \
        formwizard.h \
        workbenchintegration.h \
        designerconstants.h \
        settingspage.h \
        editorwidget.h \
        formeditorw.h \
        settingsmanager.h \
        formtemplatewizardpage.h \
        formwizarddialog.h

SOURCES += formeditorplugin.cpp \
        formeditorfactory.cpp \
        formwindoweditor.cpp \
        formwindowfile.cpp \
        formwindowhost.cpp \
        formwizard.cpp \
        workbenchintegration.cpp \
        settingspage.cpp \
        editorwidget.cpp \
        formeditorw.cpp \
        settingsmanager.cpp \
        formtemplatewizardpage.cpp \
        formwizarddialog.cpp

RESOURCES += designer.qrc
