TARGET = Mercurial
TEMPLATE = lib
include(../../qtcreatorplugin.pri)
include(mercurial_dependencies.pri)
SOURCES += mercurialplugin.cpp \
    optionspage.cpp \
    mercurialcontrol.cpp \
    mercurialclient.cpp \
    annotationhighlighter.cpp \
    mercurialeditor.cpp \
    revertdialog.cpp \
    srcdestdialog.cpp \
    mercurialcommitwidget.cpp \
    commiteditor.cpp \
    clonewizardpage.cpp \
    clonewizard.cpp \
    mercurialsettings.cpp
HEADERS += mercurialplugin.h \
    constants.h \
    optionspage.h \
    mercurialcontrol.h \
    mercurialclient.h \
    annotationhighlighter.h \
    mercurialeditor.h \
    revertdialog.h \
    srcdestdialog.h \
    mercurialcommitwidget.h \
    commiteditor.h \
    clonewizardpage.h \
    clonewizard.h \
    mercurialsettings.h
FORMS += optionspage.ui \
    revertdialog.ui \
    srcdestdialog.ui \
    mercurialcommitpanel.ui
RESOURCES += mercurial.qrc
