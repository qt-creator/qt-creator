TEMPLATE = lib
TARGET = QmlProjectManager

QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}

include(../../qtcreatorplugin.pri)
include(qmlprojectmanager_dependencies.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS += qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectfile.h \
    qmlprojectruncontrol.h \
    qmlprojectrunconfiguration.h \
    qmlprojectrunconfigurationfactory.h \
    qmlprojectapplicationwizard.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h \
    qmlprojectrunconfigurationwidget.h

SOURCES += qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectfile.cpp \
    qmlprojectruncontrol.cpp \
    qmlprojectrunconfiguration.cpp \
    qmlprojectrunconfigurationfactory.cpp \
    qmlprojectapplicationwizard.cpp \
    qmlprojectrunconfigurationwidget.cpp

RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProject.mimetypes.xml
