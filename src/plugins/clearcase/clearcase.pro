TEMPLATE = lib
TARGET = ClearCase
# PROVIDER = AudioCodes

include(../../qtcreatorplugin.pri)
include(clearcase_dependencies.pri)

HEADERS += activityselector.h \
    annotationhighlighter.h \
    checkoutdialog.h \
    clearcaseconstants.h \
    clearcasecontrol.h \
    clearcaseeditor.h \
    clearcaseplugin.h \
    clearcasesettings.h \
    clearcasesubmiteditor.h \
    clearcasesubmiteditorwidget.h \
    clearcasesync.h \
    settingspage.h \
    versionselector.h

SOURCES += activityselector.cpp \
    annotationhighlighter.cpp \
    checkoutdialog.cpp \
    clearcasecontrol.cpp \
    clearcaseeditor.cpp \
    clearcaseplugin.cpp \
    clearcasesettings.cpp \
    clearcasesubmiteditor.cpp \
    clearcasesubmiteditorwidget.cpp \
    clearcasesync.cpp \
    settingspage.cpp \
    versionselector.cpp

FORMS += checkoutdialog.ui \
    settingspage.ui \
    undocheckout.ui \
    versionselector.ui

RESOURCES += clearcase.qrc
