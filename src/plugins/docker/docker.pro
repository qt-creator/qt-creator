include(../../qtcreatorplugin.pri)

DEFINES += QT_RESTRICTED_CAST_FROM_ASCII

SOURCES += \
    dockerplugin.cpp \
    dockersettings.cpp
HEADERS += \
    dockerconstants.h \
    dockerplugin.h \
    docker_global.h \
    dockersettings.h
