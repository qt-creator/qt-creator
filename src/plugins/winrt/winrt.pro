include(../../qtcreatorplugin.pri)

HEADERS += \
    winrtconstants.h \
    winrtdeployconfiguration.h \
    winrtdevice.h \
    winrtdevicefactory.h \
    winrtpackagedeploymentstep.h \
    winrtphoneqtversion.h \
    winrtplugin.h \
    winrtqtversion.h \
    winrtqtversionfactory.h \
    winrtrunconfiguration.h \
    winrtrunconfigurationwidget.h \
    winrtruncontrol.h \
    winrtrunfactories.h

SOURCES += \
    winrtdeployconfiguration.cpp \
    winrtdevice.cpp \
    winrtdevicefactory.cpp \
    winrtpackagedeploymentstep.cpp \
    winrtphoneqtversion.cpp \
    winrtplugin.cpp \
    winrtqtversion.cpp \
    winrtqtversionfactory.cpp \
    winrtrunconfiguration.cpp \
    winrtrunconfigurationwidget.cpp \
    winrtruncontrol.cpp \
    winrtrunfactories.cpp

DEFINES += WINRT_LIBRARY

FORMS += \
    winrtrunconfigurationwidget.ui
