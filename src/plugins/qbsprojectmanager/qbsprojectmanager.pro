include(../../qtcreatorplugin.pri)

DEFINES += \
    QBSPROJECTMANAGER_LIBRARY

QT += qml

HEADERS = \
    customqbspropertiesdialog.h \
    defaultpropertyprovider.h \
    propertyprovider.h \
    qbsbuildconfiguration.h \
    qbsbuildstep.h \
    qbscleanstep.h \
    qbskitinformation.h \
    qbsinstallstep.h \
    qbsnodes.h \
    qbsnodetreebuilder.h \
    qbspmlogging.h \
    qbsprofilemanager.h \
    qbsprofilessettingspage.h \
    qbsproject.h \
    qbsprojectimporter.h \
    qbsprojectmanager_global.h \
    qbsprojectmanagerconstants.h \
    qbsprojectmanagerplugin.h \
    qbsprojectparser.h \
    qbssession.h \
    qbssettings.h

SOURCES = \
    customqbspropertiesdialog.cpp \
    defaultpropertyprovider.cpp \
    qbsbuildconfiguration.cpp \
    qbsbuildstep.cpp \
    qbscleanstep.cpp \
    qbsinstallstep.cpp \
    qbskitinformation.cpp \
    qbsnodes.cpp \
    qbsnodetreebuilder.cpp \
    qbspmlogging.cpp \
    qbsprofilemanager.cpp \
    qbsprofilessettingspage.cpp \
    qbsproject.cpp \
    qbsprojectimporter.cpp \
    qbsprojectmanagerplugin.cpp \
    qbsprojectparser.cpp \
    qbssession.cpp \
    qbssettings.cpp

FORMS = \
    customqbspropertiesdialog.ui \
    qbscleanstepconfigwidget.ui \
    qbsprofilessettingswidget.ui

RESOURCES += \
   qbsprojectmanager.qrc
