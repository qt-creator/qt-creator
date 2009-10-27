TEMPLATE = lib
TARGET = QmlProjectManager
include(../../qtcreatorplugin.pri)
include(qmlprojectmanager_dependencies.pri)
HEADERS = qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectwizard.h \
    qmlnewprojectwizard.h \
    qmltaskmanager.h \
    qmlprojectfileseditor.h
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
