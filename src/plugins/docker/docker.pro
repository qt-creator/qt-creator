include(../../qtcreatorplugin.pri)

DEFINES += QT_RESTRICTED_CAST_FROM_ASCII

SOURCES += \
    dockerdevice.cpp \
    dockerplugin.cpp \
    dockerrunconfiguration.cpp \
    dockersettings.cpp

HEADERS += \
    docker_global.h \
    dockerconstants.h \
    dockerdevice.h \
    dockerplugin.h \
    dockerrunconfiguration.h \
    dockersettings.h
