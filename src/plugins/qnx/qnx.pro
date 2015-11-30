QT += network xml

include(../../qtcreatorplugin.pri)

SOURCES += qnxplugin.cpp \
    qnxbaseqtconfigwidget.cpp \
    qnxutils.cpp \
    qnxdeviceconfigurationfactory.cpp \
    qnxdeviceconfigurationwizard.cpp \
    qnxdeviceconfigurationwizardpages.cpp \
    qnxrunconfiguration.cpp \
    qnxruncontrolfactory.cpp \
    qnxabstractrunsupport.cpp \
    qnxanalyzesupport.cpp \
    qnxdebugsupport.cpp \
    qnxdeploystepfactory.cpp \
    qnxdeployconfigurationfactory.cpp \
    qnxrunconfigurationfactory.cpp \
    qnxruncontrol.cpp \
    qnxqtversionfactory.cpp \
    qnxqtversion.cpp \
    qnxdeployconfiguration.cpp \
    qnxdeviceconfiguration.cpp \
    pathchooserdelegate.cpp \
    qnxdevicetester.cpp \
    qnxdeviceprocesssignaloperation.cpp \
    qnxdeviceprocesslist.cpp \
    qnxtoolchain.cpp \
    slog2inforunner.cpp \
    qnxattachdebugsupport.cpp \
    qnxattachdebugdialog.cpp \
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
    qnxdeviceconfigurationfactory.h \
    qnxdeviceconfigurationwizard.h \
    qnxdeviceconfigurationwizardpages.h \
    qnxrunconfiguration.h \
    qnxruncontrolfactory.h \
    qnxabstractrunsupport.h \
    qnxanalyzesupport.h \
    qnxdebugsupport.h \
    qnxdeploystepfactory.h \
    qnxdeployconfigurationfactory.h \
    qnxrunconfigurationfactory.h \
    qnxruncontrol.h \
    qnxqtversionfactory.h \
    qnxqtversion.h \
    qnxdeployconfiguration.h \
    qnxdeviceconfiguration.h \
    pathchooserdelegate.h \
    qnxdevicetester.h \
    qnxdeviceprocesssignaloperation.h \
    qnxdeviceprocesslist.h \
    qnxtoolchain.h \
    slog2inforunner.h \
    qnxattachdebugsupport.h \
    qnxattachdebugdialog.h \
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


DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII QNX_LIBRARY

RESOURCES += \
    qnx.qrc
