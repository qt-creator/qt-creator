TEMPLATE = lib
TARGET = GenericProjectManager
include(../../qtcreatorplugin.pri)
include(genericprojectmanager_dependencies.pri)
HEADERS = genericproject.h \
    genericprojectplugin.h \
    genericprojectmanager.h \
    genericprojectconstants.h \
    genericprojectnodes.h \
    genericprojectwizard.h \
    genericprojectfileseditor.h \
    pkgconfigtool.h \
    genericmakestep.h \
    genericbuildconfiguration.h
SOURCES = genericproject.cpp \
    genericprojectplugin.cpp \
    genericprojectmanager.cpp \
    genericprojectnodes.cpp \
    genericprojectwizard.cpp \
    genericprojectfileseditor.cpp \
    pkgconfigtool.cpp \
    genericmakestep.cpp \
    genericbuildconfiguration.cpp
RESOURCES += genericproject.qrc
FORMS += genericmakestep.ui
OTHER_FILES += GenericProjectManager.pluginspec
