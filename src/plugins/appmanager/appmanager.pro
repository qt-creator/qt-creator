include(../../qtcreatorplugin.pri)

HEADERS += \
    appmanagerplugin.h \
    appmanagerconstants.h \
    project/appmanagerproject.h \
    project/appmanagerprojectmanager.h \
    project/appmanagerprojectnode.h \
    project/appmanagerrunconfiguration.h \
    project/appmanagerrunconfigurationfactory.h \
    project/appmanagerruncontrol.h \
    project/appmanagerruncontrolfactory.h

SOURCES += \
    appmanagerplugin.cpp \
    project/appmanagerproject.cpp \
    project/appmanagerprojectmanager.cpp \
    project/appmanagerprojectnode.cpp \
    project/appmanagerrunconfiguration.cpp \
    project/appmanagerrunconfigurationfactory.cpp \
    project/appmanagerruncontrol.cpp \
    project/appmanagerruncontrolfactory.cpp

RESOURCES += \
    appmanager.qrc
