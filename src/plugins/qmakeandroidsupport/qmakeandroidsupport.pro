QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEANDROID_LIBRARY

HEADERS += \
    createandroidmanifestwizard.h \
    qmakeandroidsupport.h \
    qmakeandroidbuildapkstep.h \
    androidqmakebuildconfigurationfactory.h \
    qmakeandroidsupportplugin.h

SOURCES += \
    createandroidmanifestwizard.cpp \
    qmakeandroidsupport.cpp \
    qmakeandroidbuildapkstep.cpp \
    androidqmakebuildconfigurationfactory.cpp \
    qmakeandroidsupportplugin.cpp
