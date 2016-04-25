DEFINES += DESIGNER_LIBRARY

include(../../qtcreatorplugin.pri)
include(../../shared/designerintegrationv2/designerintegration.pri)
include(cpp/cpp.pri)

QT += printsupport designer designercomponents-private

HEADERS += formeditorplugin.h \
        formeditorfactory.h \
        formwindoweditor.h \
        formwindowfile.h \
        qtcreatorintegration.h \
        designerconstants.h \
        settingspage.h \
        editorwidget.h \
        formeditorw.h \
        settingsmanager.h \
        formtemplatewizardpage.h \
        codemodelhelpers.h \
        designer_export.h \
    designercontext.h \
    formeditorstack.h \
    editordata.h \
    resourcehandler.h \
    qtdesignerformclasscodegenerator.h

SOURCES += formeditorplugin.cpp \
        formeditorfactory.cpp \
        formwindoweditor.cpp \
        formwindowfile.cpp \
        qtcreatorintegration.cpp \
        settingspage.cpp \
        editorwidget.cpp \
        formeditorw.cpp \
        settingsmanager.cpp \
        formtemplatewizardpage.cpp \
        codemodelhelpers.cpp \
    designercontext.cpp \
    formeditorstack.cpp \
    resourcehandler.cpp \
    qtdesignerformclasscodegenerator.cpp

equals(TEST, 1) {
    SOURCES += gotoslot_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}

DISTFILES += README.txt
