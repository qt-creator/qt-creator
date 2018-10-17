include(../../qtcreatorplugin.pri)

HEADERS += \
    winrtconstants.h \
    winrtdebugsupport.h \
    winrtdeployconfiguration.h \
    winrtdevice.h \
    winrtdevicefactory.h \
    winrtpackagedeploymentstep.h \
    winrtphoneqtversion.h \
    winrtplugin.h \
    winrtqtversion.h \
    winrtqtversionfactory.h \
    winrtrunconfiguration.h \
    winrtruncontrol.h \
    winrtrunnerhelper.h

SOURCES += \
    winrtdebugsupport.cpp \
    winrtdeployconfiguration.cpp \
    winrtdevice.cpp \
    winrtdevicefactory.cpp \
    winrtpackagedeploymentstep.cpp \
    winrtphoneqtversion.cpp \
    winrtplugin.cpp \
    winrtqtversion.cpp \
    winrtqtversionfactory.cpp \
    winrtrunconfiguration.cpp \
    winrtruncontrol.cpp \
    winrtrunnerhelper.cpp

DEFINES += WINRT_LIBRARY

RESOURCES += \
    winrt.qrc
