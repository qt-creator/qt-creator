TEMPLATE = lib
TARGET = GenericProjectManager
include(../../qworkbenchplugin.pri)
include(genericprojectmanager_dependencies.pri)
HEADERS = genericproject.h \
    genericprojectplugin.h \
    genericprojectmanager.h \
    genericprojectconstants.h \
    genericprojectnodes.h \
    pkgconfigtool.h \
    makestep.h
SOURCES = genericproject.cpp \
    genericprojectplugin.cpp \
    genericprojectmanager.cpp \
    genericprojectnodes.cpp \
    pkgconfigtool.cpp \
    makestep.cpp
RESOURCES += genericproject.qrc
