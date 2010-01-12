TEMPLATE = lib
TARGET = QmlProjectManager
include(../../qtcreatorplugin.pri)
include(qmlprojectmanager_dependencies.pri)
DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS = qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectwizard.h \
    qmlnewprojectwizard.h \
    qmltaskmanager.h \
    qmlprojectfileseditor.h \
    qmlprojectmanager_global.h
SOURCES = qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectwizard.cpp \
    qmlnewprojectwizard.cpp \
    qmltaskmanager.cpp \
    qmlprojectfileseditor.cpp
RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProjectManager.pluginspec
