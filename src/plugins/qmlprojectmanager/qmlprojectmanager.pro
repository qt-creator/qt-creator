TEMPLATE = lib
TARGET = QmlProjectManager
include(../../qworkbenchplugin.pri)
include(qmlprojectmanager_dependencies.pri)
HEADERS = qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectwizard.h \
    qmlprojectfileseditor.h
SOURCES = qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectwizard.cpp \
    qmlprojectfileseditor.cpp
RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProjectManager.pluginspec
