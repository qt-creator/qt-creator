include(../../qtcreatorplugin.pri)

HEADERS = genericproject.h \
    genericprojectplugin.h \
    genericprojectmanager.h \
    genericprojectconstants.h \
    genericprojectnodes.h \
    genericprojectwizard.h \
    genericprojectfileseditor.h \
    genericmakestep.h \
    genericbuildconfiguration.h \
    filesselectionwizardpage.h
SOURCES = genericproject.cpp \
    genericprojectplugin.cpp \
    genericprojectmanager.cpp \
    genericprojectnodes.cpp \
    genericprojectwizard.cpp \
    genericprojectfileseditor.cpp \
    genericmakestep.cpp \
    genericbuildconfiguration.cpp \
    filesselectionwizardpage.cpp
RESOURCES += genericproject.qrc
FORMS += genericmakestep.ui

equals(TEST, 1) {
    SOURCES += genericprojectplugin_test.cpp
    DEFINES += SRCDIR=\\\"$$PWD\\\"
}
