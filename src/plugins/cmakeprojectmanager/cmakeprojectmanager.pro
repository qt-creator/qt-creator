TEMPLATE = lib
TARGET = CMakeProjectManager

include(../../qtcreatorplugin.pri)
include(cmakeprojectmanager_dependencies.pri)

HEADERS = cmakeproject.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    makestep.h \
    cmakerunconfiguration.h \
    cmakeopenprojectwizard.h \
    cmakebuildconfiguration.h \
    cmakeeditorfactory.h \
    cmakeeditor.h \
    cmakehighlighter.h \
    cmakeuicodemodelsupport.h \
    cmakelocatorfilter.h \
    cmakefilecompletionassist.h \
    cmakevalidator.h

SOURCES = cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    makestep.cpp \
    cmakerunconfiguration.cpp \
    cmakeopenprojectwizard.cpp \
    cmakebuildconfiguration.cpp \
    cmakeeditorfactory.cpp \
    cmakeeditor.cpp \
    cmakehighlighter.cpp \
    cmakeuicodemodelsupport.cpp \
    cmakelocatorfilter.cpp \
    cmakefilecompletionassist.cpp \
    cmakevalidator.cpp

RESOURCES += cmakeproject.qrc

FORMS +=

OTHER_FILES += CMakeProject.mimetypes.xml
