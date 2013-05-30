QT += network xml
PROVIDER = BlackBerry

include(../../qtcreatorplugin.pri)

SOURCES += qnxplugin.cpp \
    blackberryqtversionfactory.cpp \
    blackberryqtversion.cpp \
    qnxbaseqtconfigwidget.cpp \
    blackberrydeployconfigurationfactory.cpp \
    blackberrydeployconfiguration.cpp \
    blackberrycreatepackagestep.cpp \
    blackberrycreatepackagestepconfigwidget.cpp \
    blackberrycreatepackagestepfactory.cpp \
    blackberrydeploystep.cpp \
    blackberrydeployconfigurationwidget.cpp \
    blackberrydeploystepconfigwidget.cpp \
    blackberrydeviceconfigurationfactory.cpp \
    blackberrydeviceconfigurationwizard.cpp \
    blackberrydeviceconfigurationwizardpages.cpp \
    blackberrydeploystepfactory.cpp \
    blackberryrunconfiguration.cpp \
    blackberryrunconfigurationwidget.cpp \
    blackberryrunconfigurationfactory.cpp \
    blackberryruncontrolfactory.cpp \
    blackberryruncontrol.cpp \
    blackberrydebugsupport.cpp \
    blackberryapplicationrunner.cpp \
    qnxutils.cpp \
    blackberrydeviceconfigurationwidget.cpp \
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
    qnxabstractqtversion.cpp \
    blackberrydeviceconfiguration.cpp \
    qnxdeployconfiguration.cpp \
    qnxdeviceconfiguration.cpp \
    blackberrydeployinformation.cpp \
    pathchooserdelegate.cpp \
    blackberryabstractdeploystep.cpp \
    blackberryndksettingswidget.cpp \
    blackberryndksettingspage.cpp \
    blackberryconfiguration.cpp \
    bardescriptormagicmatcher.cpp \
    bardescriptoreditorfactory.cpp \
    bardescriptoreditor.cpp \
    bardescriptoreditorwidget.cpp \
    bardescriptordocument.cpp \
    bardescriptordocumentnodehandlers.cpp \
    bardescriptorpermissionsmodel.cpp \
    blackberrykeyswidget.cpp \
    blackberrykeyspage.cpp \
    blackberrycsjregistrar.cpp \
    blackberrycertificate.cpp \
    blackberrycertificatemodel.cpp \
    blackberryregisterkeydialog.cpp \
    blackberryimportcertificatedialog.cpp \
    blackberrycreatecertificatedialog.cpp \
    blackberrydebugtokenrequester.cpp \
    blackberrydebugtokenrequestdialog.cpp \
    blackberrydebugtokenuploader.cpp \
    blackberrydebugtokenreader.cpp \
    blackberryndkprocess.cpp \
    blackberrydeviceprocesssupport.cpp \
    blackberrycheckdevmodestepfactory.cpp \
    blackberrycheckdevmodestep.cpp \
    blackberrycheckdevmodestepconfigwidget.cpp \
    blackberrydeviceconnection.cpp \
    blackberrydeviceconnectionmanager.cpp \
    blackberrydeviceinformation.cpp \
    blackberrysshkeysgenerator.cpp \
    blackberryprocessparser.cpp \
    blackberrysigningpasswordsdialog.cpp \
    bardescriptoreditorpackageinformationwidget.cpp \
    bardescriptoreditorauthorinformationwidget.cpp \
    bardescriptoreditorentrypointwidget.cpp \
    bardescriptoreditorgeneralwidget.cpp \
    bardescriptoreditorpermissionswidget.cpp \
    bardescriptoreditorenvironmentwidget.cpp \
    bardescriptoreditorassetswidget.cpp \
    bardescriptoreditorabstractpanelwidget.cpp \
    blackberrysetupwizard.cpp \
    blackberrysetupwizardpages.cpp \
    blackberryutils.cpp \
    qnxdevicetester.cpp

HEADERS += qnxplugin.h\
    qnxconstants.h \
    blackberryqtversionfactory.h \
    blackberryqtversion.h \
    qnxbaseqtconfigwidget.h \
    blackberrydeployconfigurationfactory.h \
    blackberrydeployconfiguration.h \
    blackberrycreatepackagestep.h \
    blackberrycreatepackagestepconfigwidget.h \
    blackberrycreatepackagestepfactory.h \
    blackberrydeploystep.h \
    blackberrydeployconfigurationwidget.h \
    blackberrydeploystepconfigwidget.h \
    blackberrydeviceconfigurationfactory.h \
    blackberrydeviceconfigurationwizard.h \
    blackberrydeviceconfigurationwizardpages.h \
    blackberrydeploystepfactory.h \
    blackberryrunconfiguration.h \
    blackberryrunconfigurationwidget.h \
    blackberryrunconfigurationfactory.h \
    blackberryruncontrolfactory.h \
    blackberryruncontrol.h \
    blackberrydebugsupport.h \
    blackberryapplicationrunner.h \
    qnxutils.h \
    blackberrydeviceconfigurationwidget.h \
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
    qnxabstractqtversion.h \
    blackberrydeviceconfiguration.h \
    qnxdeployconfiguration.h \
    qnxdeviceconfiguration.h \
    blackberrydeployinformation.h \
    pathchooserdelegate.h \
    blackberryabstractdeploystep.h \
    blackberryndksettingswidget.h \
    blackberryndksettingspage.h \
    blackberryconfiguration.h \
    bardescriptormagicmatcher.h \
    bardescriptoreditorfactory.h \
    bardescriptoreditor.h \
    bardescriptoreditorwidget.h \
    bardescriptordocument.h \
    bardescriptordocumentnodehandlers.h \
    bardescriptorpermissionsmodel.h \
    blackberrykeyswidget.h \
    blackberrykeyspage.h \
    blackberrycsjregistrar.h \
    blackberrycertificate.h \
    blackberrycertificatemodel.h \
    blackberryregisterkeydialog.h \
    blackberryimportcertificatedialog.h \
    blackberrycreatecertificatedialog.h \
    blackberrydebugtokenrequester.h \
    blackberrydebugtokenrequestdialog.h \
    blackberrydebugtokenuploader.h \
    blackberrydebugtokenreader.h \
    blackberryndkprocess.h \
    blackberrydeviceprocesssupport.h \
    blackberrycheckdevmodestepfactory.h \
    blackberrycheckdevmodestep.h \
    blackberrycheckdevmodestepconfigwidget.h \
    blackberrydeviceconnection.h \
    blackberrydeviceconnectionmanager.h \
    blackberrydeviceinformation.h \
    blackberrysshkeysgenerator.h \
    blackberryprocessparser.h \
    blackberrysigningpasswordsdialog.h \
    bardescriptoreditorpackageinformationwidget.h \
    bardescriptoreditorauthorinformationwidget.h \
    bardescriptoreditorentrypointwidget.h \
    bardescriptoreditorgeneralwidget.h \
    bardescriptoreditorpermissionswidget.h \
    bardescriptoreditorenvironmentwidget.h \
    bardescriptoreditorassetswidget.h \
    bardescriptoreditorabstractpanelwidget.h \
    blackberrysetupwizard.h \
    blackberrysetupwizardpages.h \
    blackberryutils.h \
    qnxdevicetester.h

FORMS += \
    blackberrydeviceconfigurationwizardsetuppage.ui \
    blackberryrunconfigurationwidget.ui \
    blackberrydeviceconfigurationwizardsshkeypage.ui \
    blackberrydeployconfigurationwidget.ui \
    blackberrydeviceconfigurationwidget.ui \
    qnxbaseqtconfigwidget.ui \
    blackberryndksettingswidget.ui \
    blackberrykeyswidget.ui \
    blackberryregisterkeydialog.ui \
    blackberryimportcertificatedialog.ui \
    blackberrycreatecertificatedialog.ui \
    blackberrydebugtokenrequestdialog.ui \
    blackberrycreatepackagestepconfigwidget.ui \
    blackberrysigningpasswordsdialog.ui \
    bardescriptoreditorpackageinformationwidget.ui \
    bardescriptoreditorauthorinformationwidget.ui \
    bardescriptoreditorentrypointwidget.ui \
    bardescriptoreditorgeneralwidget.ui \
    bardescriptoreditorpermissionswidget.ui \
    bardescriptoreditorenvironmentwidget.ui \
    bardescriptoreditorassetswidget.ui \
    blackberrysetupwizardkeyspage.ui \
    blackberrysetupwizarddevicepage.ui \
    blackberrysetupwizardfinishpage.ui

include(../../private_headers.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += gui-private
    DEFINES += QNX_ZIP_FILE_SUPPORT
} else {
    exists($${QT_PRIVATE_HEADERS}/QtGui/private/qzipreader_p.h) {
        DEFINES += QNX_ZIP_FILE_SUPPORT
    } else {
        warning("The QNX plugin depends on private headers from QtGui module, to be fully functional.")
        warning("To fix it, pass 'QT_PRIVATE_HEADERS=$QTDIR/include' to qmake, where $QTDIR is the source directory of qt.")
    }
}

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

RESOURCES += \
    qnx.qrc
