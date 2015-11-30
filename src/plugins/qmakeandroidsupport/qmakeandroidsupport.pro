QT += network
include(../../qtcreatorplugin.pri)

DEFINES += \
    QMAKEANDROID_LIBRARY

HEADERS += \
    androidextralibrarylistmodel.h \
    androidpackageinstallationfactory.h \
    androidpackageinstallationstep.h \
    createandroidmanifestwizard.h \
    qmakeandroidsupport.h \
    qmakeandroidrunconfiguration.h \
    qmakeandroidrunfactories.h \
    qmakeandroidbuildapkstep.h \
    qmakeandroidbuildapkwidget.h \
    androidqmakebuildconfigurationfactory.h \
    qmakeandroidsupportplugin.h

SOURCES += \
    androidextralibrarylistmodel.cpp \
    androidpackageinstallationfactory.cpp \
    androidpackageinstallationstep.cpp \
    createandroidmanifestwizard.cpp \
    qmakeandroidsupport.cpp \
    qmakeandroidrunconfiguration.cpp \
    qmakeandroidrunfactories.cpp \
    qmakeandroidbuildapkstep.cpp \
    qmakeandroidbuildapkwidget.cpp \
    androidqmakebuildconfigurationfactory.cpp \
    qmakeandroidsupportplugin.cpp

FORMS += qmakeandroidbuildapkwidget.ui

RESOURCES +=

