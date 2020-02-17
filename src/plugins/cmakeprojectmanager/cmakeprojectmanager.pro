DEFINES += CMAKEPROJECTMANAGER_LIBRARY
include(../../qtcreatorplugin.pri)

HEADERS = builddirmanager.h \
    builddirparameters.h \
    builddirreader.h \
    cmakebuildstep.h \
    cmakebuildsystem.h \
    cmakebuildtarget.h \
    cmakeconfigitem.h \
    cmakeprocess.h \
    cmakeproject.h \
    cmakeprojectimporter.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    cmakebuildconfiguration.h \
    cmakeeditor.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmaketool.h \
    cmaketoolsettingsaccessor.h \
    cmakeparser.h \
    cmakesettingspage.h \
    cmaketoolmanager.h \
    cmake_global.h \
    cmakekitinformation.h \
    cmakecbpparser.h \
    cmakebuildsettingswidget.h \
    cmakeindenter.h \
    cmakeautocompleter.h \
    cmakespecificsettings.h \
    cmakespecificsettingspage.h \
    configmodel.h \
    configmodelitemdelegate.h \
    fileapidataextractor.h \
    fileapiparser.h \
    fileapireader.h \
    projecttreehelper.h \
    servermode.h \
    servermodereader.h

SOURCES = builddirmanager.cpp \
    builddirparameters.cpp \
    builddirreader.cpp \
    cmakebuildstep.cpp \
    cmakebuildsystem.cpp \
    cmakeconfigitem.cpp \
    cmakeprocess.cpp \
    cmakeproject.cpp \
    cmakeprojectimporter.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditor.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmaketool.cpp \
    cmaketoolsettingsaccessor.cpp \
    cmakeparser.cpp \
    cmakesettingspage.cpp \
    cmaketoolmanager.cpp \
    cmakekitinformation.cpp \
    cmakecbpparser.cpp \
    cmakebuildsettingswidget.cpp \
    cmakeindenter.cpp \
    cmakeautocompleter.cpp \
    cmakespecificsettings.cpp \
    cmakespecificsettingspage.cpp \
    configmodel.cpp \
    configmodelitemdelegate.cpp \
    fileapidataextractor.cpp \
    fileapiparser.cpp \
    fileapireader.cpp \
    projecttreehelper.cpp \
    servermode.cpp \
    servermodereader.cpp

RESOURCES += cmakeproject.qrc

FORMS += \
    cmakespecificsettingspage.ui
