include(../../qtcreatorplugin.pri)

HEADERS += annotationhighlighter.h \
    cvsplugin.h \
    cvsclient.h \
    cvscontrol.h \
    settingspage.h \
    cvseditor.h \
    cvssubmiteditor.h \
    cvssettings.h \
    cvsutils.h \
    cvsconstants.h

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
