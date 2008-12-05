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
    makestep.h
SOURCES = cmakeproject.cpp \
    cmakeprojectplugin.cpp \
    cmakeprojectmanager.cpp \
    cmakeprojectnodes.cpp \
    cmakestep.cpp \
    makestep.cpp
RESOURCES += cmakeproject.qrc
