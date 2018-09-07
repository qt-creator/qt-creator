QT += network

include(../../qtcreatorplugin.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY
HEADERS += qmlproject.h \
    qmlprojectenvironmentaspect.h \
    qmlprojectplugin.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectrunconfiguration.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h

SOURCES += qmlproject.cpp \
    qmlprojectenvironmentaspect.cpp \
    qmlprojectplugin.cpp \
    qmlprojectnodes.cpp \
    qmlprojectrunconfiguration.cpp

RESOURCES += qmlproject.qrc
