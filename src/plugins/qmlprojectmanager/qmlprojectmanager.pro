QT += network

include(../../qtcreatorplugin.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS += qmlproject.h \
    qmlprojectenvironmentaspect.h \
    qmlprojectplugin.h \
    qmlprojectmanager.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectfile.h \
    qmlprojectrunconfiguration.h \
    qmlprojectrunconfigurationfactory.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h \
    qmlprojectrunconfigurationwidget.h \
    qmlapp.h \
    qmlapplicationwizard.h \
    qmlapplicationwizardpages.h

SOURCES += qmlproject.cpp \
    qmlprojectenvironmentaspect.cpp \
    qmlprojectplugin.cpp \
    qmlprojectmanager.cpp \
    qmlprojectnodes.cpp \
    qmlprojectfile.cpp \
    qmlprojectrunconfiguration.cpp \
    qmlprojectrunconfigurationfactory.cpp \
    qmlprojectrunconfigurationwidget.cpp \
    qmlapp.cpp \
    qmlapplicationwizard.cpp \
    qmlapplicationwizardpages.cpp

RESOURCES += qmlproject.qrc
