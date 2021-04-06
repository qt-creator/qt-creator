include(../../qtcreatorplugin.pri)

HEADERS += \
    perforceplugin.h \
    perforcechecker.h \
    perforceeditor.h \
    changenumberdialog.h \
    perforcesubmiteditor.h \
    pendingchangesdialog.h \
    perforcesettings.h \
    annotationhighlighter.h \
    perforcesubmiteditorwidget.h

SOURCES += perforceplugin.cpp \
    perforcechecker.cpp \
    perforceeditor.cpp \
    changenumberdialog.cpp \
    perforcesubmiteditor.cpp \
    pendingchangesdialog.cpp \
    perforcesettings.cpp \
    annotationhighlighter.cpp \
    perforcesubmiteditorwidget.cpp

FORMS += \
    changenumberdialog.ui \
    pendingchangesdialog.ui \
    submitpanel.ui
