TEMPLATE = lib
TARGET = CVS

include(../../qtcreatorplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/vcsbase/vcsbase.pri)
include(../../libs/utils/utils.pri)

HEADERS += annotationhighlighter.h \
    cvsplugin.h \
    cvscontrol.h \
    settingspage.h \
    cvseditor.h \
    cvssubmiteditor.h \
    cvssettings.h \
    cvsutils.h \
    cvsconstants.h \
    checkoutwizard.h  \
    checkoutwizardpage.h

SOURCES += annotationhighlighter.cpp \
    cvsplugin.cpp \
    cvscontrol.cpp \
    settingspage.cpp \
    cvseditor.cpp \
    cvssubmiteditor.cpp \
    cvssettings.cpp \
    cvsutils.cpp \
    checkoutwizard.cpp \
    checkoutwizardpage.cpp

FORMS += settingspage.ui

RESOURCES += cvs.qrc
