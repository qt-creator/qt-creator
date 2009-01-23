TEMPLATE = lib
TARGET = CMakeProjectManager
include(../../qworkbenchplugin.pri)
include(cmakeprojectmanager_dependencies.pri)
HEADERS = cmakeproject.h \
    cmakeprojectplugin.h \
    cmakeprojectmanager.h \
    cmakeprojectconstants.h \
    cmakeprojectnodes.h \
    cmakestep.h \
    makestep.h \
    cmakerunconfiguration.h \
    cmakeconfigurewidget.h
SOURCES = cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    cmakestep.cpp \
    makestep.cpp \
    cmakerunconfiguration.cpp \
    cmakeconfigurewidget.cpp
RESOURCES += cmakeproject.qrc
FORMS += cmakeconfigurewidget.ui
