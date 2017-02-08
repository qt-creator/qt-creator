include(../../qtcreatorplugin.pri)

HEADERS += \
    perforceplugin.h \
    perforcechecker.h \
    settingspage.h \
    perforceeditor.h \
    changenumberdialog.h \
    perforcesubmiteditor.h \
    pendingchangesdialog.h \
    perforceversioncontrol.h \
    perforcesettings.h \
    annotationhighlighter.h \
    perforcesubmiteditorwidget.h

SOURCES += perforceplugin.cpp \
    perforcechecker.cpp \
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
