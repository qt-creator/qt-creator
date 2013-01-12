TEMPLATE = lib
TARGET = QmlProjectManager

QT += network declarative

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
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h \
    qmlprojectrunconfigurationwidget.h \
    qmlapp.h \
    qmlapplicationwizard.h

SOURCES += qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectfile.cpp \
    qmlprojectruncontrol.cpp \
    qmlprojectrunconfiguration.cpp \
    qmlprojectrunconfigurationfactory.cpp \
    qmlprojectrunconfigurationwidget.cpp \
    qmlapp.cpp \
    qmlapplicationwizard.cpp

RESOURCES += qmlproject.qrc

OTHER_FILES += QmlProject.mimetypes.xml
