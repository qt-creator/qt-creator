TEMPLATE = lib
TARGET = ClearCase
# PROVIDER = AudioCodes

include(../../qtcreatorplugin.pri)
include(clearcase_dependencies.pri)

DEFINES += QT_NO_CAST_FROM_ASCII

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
    settingspage.cpp \
    versionselector.cpp

FORMS += checkoutdialog.ui \
    settingspage.ui \
    undocheckout.ui \
    versionselector.ui

RESOURCES += clearcase.qrc
