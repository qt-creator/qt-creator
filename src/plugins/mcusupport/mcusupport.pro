include(../../qtcreatorplugin.pri)

DEFINES += MCUSUPPORT_LIBRARY

QT += gui network

HEADERS += \
    mcusupport_global.h \
    mcusupportconstants.h \
    mcusupportdevice.h \
    mcusupportoptions.h \
    mcusupportoptionspage.h \
    mcusupportplugin.h \
    mcusupportsdk.h \
    mcusupportrunconfiguration.h

SOURCES += \
    mcusupportdevice.cpp \
    mcusupportoptions.cpp \
    mcusupportoptionspage.cpp \
    mcusupportplugin.cpp \
    mcusupportsdk.cpp \
    mcusupportrunconfiguration.cpp

RESOURCES += \
    mcusupport.qrc
