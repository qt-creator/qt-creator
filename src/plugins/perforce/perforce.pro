TEMPLATE = lib
TARGET = Perforce

include(../../qworkbenchplugin.pri)
include(perforce_dependencies.pri)

HEADERS += p4.h \
    perforceplugin.h \
    perforceoutputwindow.h \
    settingspage.h \
    perforceeditor.h \
    changenumberdialog.h \
    perforcesubmiteditor.h \
    pendingchangesdialog.h \
    perforceconstants.h \
    perforceversioncontrol.h \
    perforcesettings.h \
    annotationhighlighter.h \
    perforcesubmiteditorwidget.h

SOURCES += perforceplugin.cpp \
    perforceoutputwindow.cpp \
    settingspage.cpp \
    perforceeditor.cpp \
    changenumberdialog.cpp \
    perforcesubmiteditor.cpp \
    pendingchangesdialog.cpp \
    perforceversioncontrol.cpp \
    perforcesettings.cpp \
    annotationhighlighter.cpp \
    perforcesubmiteditorwidget.cpp

FORMS += settingspage.ui \
    changenumberdialog.ui \
    pendingchangesdialog.ui \
    submitpanel.ui

RESOURCES += perforce.qrc
