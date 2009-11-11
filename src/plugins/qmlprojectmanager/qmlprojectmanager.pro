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
    qmlprojectfileseditor.h \
    qmltaskmanager.h \
    qmlmakestep.h
SOURCES = qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectwizard.cpp \
    qmlnewprojectwizard.cpp \
    qmlprojectfileseditor.cpp \
    qmltaskmanager.cpp \
    qmlmakestep.cpp
RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProjectManager.pluginspec
