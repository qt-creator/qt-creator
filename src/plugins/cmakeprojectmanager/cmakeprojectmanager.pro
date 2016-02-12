DEFINES += CMAKEPROJECTMANAGER_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS = cmakebuildinfo.h \
    cmakebuildstep.h \
    cmakeconfigitem.h \
    cmakeproject.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
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
    cmakeautocompleter.h \
    cmakepreloadcachekitinformation.h \
    cmakepreloadcachekitconfigwidget.h

SOURCES = cmakebuildstep.cpp \
    cmakeconfigitem.cpp \
    cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
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
    cmakeautocompleter.cpp \
    cmakepreloadcachekitinformation.cpp \
    cmakepreloadcachekitconfigwidget.cpp

RESOURCES += cmakeproject.qrc
