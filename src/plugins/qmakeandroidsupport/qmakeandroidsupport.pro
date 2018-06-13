QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEANDROID_LIBRARY

HEADERS += \
    createandroidmanifestwizard.h \
    qmakeandroidsupport.h \
    qmakeandroidbuildapkstep.h \
    qmakeandroidbuildapkwidget.h \
    androidqmakebuildconfigurationfactory.h \
    qmakeandroidsupportplugin.h

SOURCES += \
    createandroidmanifestwizard.cpp \
    qmakeandroidsupport.cpp \
    qmakeandroidbuildapkstep.cpp \
    qmakeandroidbuildapkwidget.cpp \
    androidqmakebuildconfigurationfactory.cpp \
    qmakeandroidsupportplugin.cpp
