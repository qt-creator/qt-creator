include(../../qtcreatorplugin.pri)

HEADERS = genericproject.h \
    genericprojectplugin.h \
    genericprojectconstants.h \
    genericprojectnodes.h \
    genericprojectwizard.h \
    genericprojectfileseditor.h \
    genericmakestep.h \
    genericbuildconfiguration.h \
    filesselectionwizardpage.h
SOURCES = genericproject.cpp \
    genericprojectplugin.cpp \
    genericprojectnodes.cpp \
    genericprojectwizard.cpp \
    genericprojectfileseditor.cpp \
    genericmakestep.cpp \
    genericbuildconfiguration.cpp \
    filesselectionwizardpage.cpp
FORMS += genericmakestep.ui

equals(TEST, 1) {
    SOURCES += genericprojectplugin_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
