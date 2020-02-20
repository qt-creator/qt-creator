DEFINES += BOOT2QT_LIBRARY
QT += network

include(../../qtcreatorplugin.pri)
include(boot2qt_dependencies.pri)

include(device-detection/device-detection.pri)

HEADERS += \
    qdbutils.h \
    qdbdevice.h \
    qdbqtversion.h \
    qdbdeployconfigurationfactory.h \
    qdbrunconfiguration.h \
    qdbmakedefaultappstep.h \
    qdbmakedefaultappservice.h \
    qdbstopapplicationstep.h \
    qdbstopapplicationservice.h \
    qdbdevicedebugsupport.h \
    qdbconstants.h \
    qdb_global.h \
    qdbplugin.h

SOURCES += \
    qdbutils.cpp \
    qdbdevice.cpp \
    qdbqtversion.cpp \
    qdbdeployconfigurationfactory.cpp \
    qdbrunconfiguration.cpp \
    qdbmakedefaultappstep.cpp \
    qdbmakedefaultappservice.cpp \
    qdbstopapplicationstep.cpp \
    qdbstopapplicationservice.cpp \
    qdbdevicedebugsupport.cpp \
    qdbplugin.cpp \

RESOURCES += \
    qdb.qrc
