DEFINES += CMAKEPROJECTMANAGER_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS = builddirmanager.h \
    builddirreader.h \
    cmakebuildinfo.h \
    cmakebuildstep.h \
    cmakeconfigitem.h \
    cmakeproject.h \
    cmakeprojectimporter.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    cmakerunconfiguration.h \
    cmakebuildconfiguration.h \
    cmakeeditor.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmaketool.h \
    cmakeparser.h \
    cmakesettingspage.h \
    cmakesnippetprovider.h \
    cmaketoolmanager.h \
    cmake_global.h \
    cmakekitinformation.h \
    cmakekitconfigwidget.h \
    cmakecbpparser.h \
    cmakebuildsettingswidget.h \
    cmakeindenter.h \
    cmakeautocompleter.h \
    configmodel.h \
    configmodelitemdelegate.h \
    servermode.h \
    servermodereader.h \
    tealeafreader.h \
    treescanner.h

SOURCES = builddirmanager.cpp \
    builddirreader.cpp \
    cmakebuildstep.cpp \
    cmakeconfigitem.cpp \
    cmakeproject.cpp \
    cmakeprojectimporter.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    cmakerunconfiguration.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditor.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmaketool.cpp \
    cmakeparser.cpp \
    cmakesettingspage.cpp \
    cmakesnippetprovider.cpp \
    cmaketoolmanager.cpp \
    cmakekitinformation.cpp \
    cmakekitconfigwidget.cpp \
    cmakecbpparser.cpp \
    cmakebuildsettingswidget.cpp \
    cmakeindenter.cpp \
    cmakeautocompleter.cpp \
    configmodel.cpp \
    configmodelitemdelegate.cpp \
    servermode.cpp \
    servermodereader.cpp \
    tealeafreader.cpp \
    treescanner.cpp

RESOURCES += cmakeproject.qrc
