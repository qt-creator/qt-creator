DEFINES += CMAKEPROJECTMANAGER_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS = cmakebuildinfo.h \
    cmakeproject.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    makestep.h \
    cmakerunconfiguration.h \
    cmakeopenprojectwizard.h \
    cmakebuildconfiguration.h \
    cmakeeditor.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmaketool.h \
    cmakeparser.h \
    generatorinfo.h \
    cmakesettingspage.h \
    cmaketoolmanager.h \
    cmake_global.h \
    cmakekitinformation.h \
    cmakekitconfigwidget.h \
    cmakecbpparser.h \
    cmakefile.h \
    cmakebuildsettingswidget.h \
    cmakeindenter.h \
    cmakeautocompleter.h

SOURCES = cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    makestep.cpp \
    cmakerunconfiguration.cpp \
    cmakeopenprojectwizard.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditor.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmaketool.cpp \
    cmakeparser.cpp \
    generatorinfo.cpp \
    cmakesettingspage.cpp \
    cmaketoolmanager.cpp \
    cmakekitinformation.cpp \
    cmakekitconfigwidget.cpp \
    cmakecbpparser.cpp \
    cmakefile.cpp \
    cmakebuildsettingswidget.cpp \
    cmakeindenter.cpp \
    cmakeautocompleter.cpp


RESOURCES += cmakeproject.qrc
