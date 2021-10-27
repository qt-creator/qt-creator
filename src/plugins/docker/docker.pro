include(../../qtcreatorplugin.pri)

DEFINES += QT_RESTRICTED_CAST_FROM_ASCII

SOURCES += \
    dockerbuildstep.cpp \
    dockerdevice.cpp \
    dockerplugin.cpp \
    dockersettings.cpp

HEADERS += \
    docker_global.h \
    dockerbuildstep.h \
    dockerconstants.h \
    dockerdevice.h \
    dockerplugin.h \
    dockersettings.h
