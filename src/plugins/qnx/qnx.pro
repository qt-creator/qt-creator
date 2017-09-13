QT += network xml

include(../../qtcreatorplugin.pri)

SOURCES += qnxplugin.cpp \
    qnxbaseqtconfigwidget.cpp \
    qnxutils.cpp \
    qnxdevicefactory.cpp \
    qnxdevicewizard.cpp \
    qnxrunconfiguration.cpp \
    qnxanalyzesupport.cpp \
    qnxdebugsupport.cpp \
    qnxdeploystepfactory.cpp \
    qnxdeployconfigurationfactory.cpp \
    qnxrunconfigurationfactory.cpp \
    qnxqtversionfactory.cpp \
    qnxqtversion.cpp \
    qnxdeployconfiguration.cpp \
    qnxdevice.cpp \
    pathchooserdelegate.cpp \
    qnxdevicetester.cpp \
    qnxdeviceprocesssignaloperation.cpp \
    qnxdeviceprocesslist.cpp \
    qnxtoolchain.cpp \
    slog2inforunner.cpp \
    qnxconfiguration.cpp \
    qnxsettingswidget.cpp \
    qnxconfigurationmanager.cpp \
    qnxsettingspage.cpp \
    qnxversionnumber.cpp \
    qnxdeployqtlibrariesdialog.cpp \
    qnxdeviceprocess.cpp

HEADERS += qnxplugin.h\
    qnxconstants.h \
    qnxbaseqtconfigwidget.h \
    qnxutils.h \
    qnxdevicefactory.h \
    qnxdevicewizard.h \
    qnxrunconfiguration.h \
    qnxanalyzesupport.h \
    qnxdebugsupport.h \
    qnxdeploystepfactory.h \
    qnxdeployconfigurationfactory.h \
    qnxrunconfigurationfactory.h \
    qnxqtversionfactory.h \
    qnxqtversion.h \
    qnxdeployconfiguration.h \
    qnxdevice.h \
    pathchooserdelegate.h \
    qnxdevicetester.h \
    qnxdeviceprocesssignaloperation.h \
    qnxdeviceprocesslist.h \
    qnxtoolchain.h \
    slog2inforunner.h \
    qnxconfiguration.h \
    qnxsettingswidget.h \
    qnxconfigurationmanager.h \
    qnxsettingspage.h \
    qnxversionnumber.h \
    qnxdeployqtlibrariesdialog.h \
    qnx_export.h \
    qnxdeviceprocess.h

FORMS += \
    qnxsettingswidget.ui \
    qnxdeployqtlibrariesdialog.ui


DEFINES += QNX_LIBRARY

RESOURCES += \
    qnx.qrc
