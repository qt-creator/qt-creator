TEMPLATE = lib
TARGET = ClearCase
# PROVIDER = AudioCodes

include(../../qtcreatorplugin.pri)
include(clearcase_dependencies.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

HEADERS += annotationhighlighter.h \
    clearcaseplugin.h \
    clearcasecontrol.h \
    settingspage.h \
    clearcaseeditor.h \
    clearcasesettings.h \
    clearcaseconstants.h \
    clearcasesubmiteditor.h \
    checkoutdialog.h \
    activityselector.h \
    clearcasesubmiteditorwidget.h \
    versionselector.h

SOURCES += annotationhighlighter.cpp \
    clearcaseplugin.cpp \
    clearcasecontrol.cpp \
    settingspage.cpp \
    clearcaseeditor.cpp \
    clearcasesettings.cpp \
    clearcasesubmiteditor.cpp \
    checkoutdialog.cpp \
    activityselector.cpp \
    clearcasesubmiteditorwidget.cpp \
    versionselector.cpp

FORMS += settingspage.ui \
    checkoutdialog.ui \
    undocheckout.ui \
    versionselector.ui

RESOURCES += clearcase.qrc
