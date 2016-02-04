include(../../qtcreatorplugin.pri)

HEADERS += annotationhighlighter.h \
    cvsplugin.h \
    cvsclient.h \
    cvscontrol.h \
    settingspage.h \
    cvseditor.h \
    cvssubmiteditor.h \
    cvssettings.h \
    cvsutils.h

SOURCES += annotationhighlighter.cpp \
    cvsplugin.cpp \
    cvsclient.cpp \
    cvscontrol.cpp \
    settingspage.cpp \
    cvseditor.cpp \
    cvssubmiteditor.cpp \
    cvssettings.cpp \
    cvsutils.cpp

FORMS += settingspage.ui

RESOURCES += cvs.qrc
