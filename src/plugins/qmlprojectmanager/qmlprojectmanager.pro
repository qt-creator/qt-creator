TEMPLATE = lib
TARGET = QmlProjectManager

QT += declarative

include(../../qtcreatorplugin.pri)
include(qmlprojectmanager_dependencies.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS += qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectwizard.h \
    qmlprojectfile.h \
    qmlprojectruncontrol.h \
    qmlprojectrunconfiguration.h \
    qmlprojectrunconfigurationfactory.h \
    qmlnewprojectwizard.h \
    qmltaskmanager.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h \
    qmlprojecttarget.h
SOURCES += qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectwizard.cpp \
    qmlprojectfile.cpp \
    qmlprojectruncontrol.cpp \
    qmlprojectrunconfiguration.cpp \
    qmlprojectrunconfigurationfactory.cpp \
    qmlnewprojectwizard.cpp \
    qmltaskmanager.cpp \
    qmlprojecttarget.cpp
RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProjectManager.pluginspec
