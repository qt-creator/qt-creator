QT += network

include(../../qtcreatorplugin.pri)
include(fileformat/fileformat.pri)

DEFINES += QMLPROJECTMANAGER_LIBRARY

HEADERS += \
    qmlmainfileaspect.h \
    qmlproject.h \
    qmlprojectplugin.h \
    qmlprojectconstants.h \
    qmlprojectnodes.h \
    qmlprojectrunconfiguration.h \
    qmlprojectmanager_global.h \
    qmlprojectmanagerconstants.h

SOURCES += \
    qmlmainfileaspect.cpp \
    qmlproject.cpp \
    qmlprojectplugin.cpp \
    qmlprojectnodes.cpp \
    qmlprojectrunconfiguration.cpp

RESOURCES += qmlproject.qrc
