include(../../qtcreatorplugin.pri)

isEmpty(ANDROID_ENABLE):ANDROID_EXPERIMENTAL_STR="true"
else:ANDROID_EXPERIMENTAL_STR="false"

QT += xml network

HEADERS += \
    androidqtsupport.h \
    androidconstants.h \
    androidconfigurations.h \
    androidmanager.h \
    androidrunconfiguration.h \
    androidruncontrol.h \
    androidsettingspage.h \
    androidsettingswidget.h \
    androidtoolchain.h \
    androiderrormessage.h \
    androidglobal.h \
    androidrunner.h \
    androiddebugsupport.h \
    androidqtversionfactory.h \
    androidqtversion.h \
    androiddeployconfiguration.h \
    androidcreatekeystorecertificate.h \
    javaparser.h \
    androidplugin.h \
    androiddevicefactory.h \
    androiddevice.h \
    androidgdbserverkitinformation.h \
    androidanalyzesupport.h \
    androidmanifesteditorfactory.h \
    androidmanifesteditor.h \
    androidmanifesteditorwidget.h \
    androidmanifestdocument.h \
    androiddevicedialog.h \
    androiddeployqtstep.h \
    certificatesmodel.h \
    androiddeployqtwidget.h \
    androidpotentialkit.h \
    androidsignaloperation.h \
    javaeditor.h \
    javaindenter.h \
    avddialog.h \
    android_global.h \
    androidbuildapkstep.h \
    androidbuildapkwidget.h \
    androidrunnable.h \
    androidtoolmanager.h \
    androidsdkmanager.h \
    androidavdmanager.h \
    androidrunconfigurationwidget.h \
    adbcommandswidget.h \
    androidsdkpackage.h \
    androidsdkmodel.h \
    androidsdkmanagerwidget.h

SOURCES += \
    androidconfigurations.cpp \
    androidmanager.cpp \
    androidrunconfiguration.cpp \
    androidruncontrol.cpp \
    androidsettingspage.cpp \
    androidsettingswidget.cpp \
    androidtoolchain.cpp \
    androiderrormessage.cpp \
    androidrunner.cpp \
    androiddebugsupport.cpp \
    androidqtversionfactory.cpp \
    androidqtversion.cpp \
    androiddeployconfiguration.cpp \
    androidcreatekeystorecertificate.cpp \
    javaparser.cpp \
    androidplugin.cpp \
    androiddevicefactory.cpp \
    androiddevice.cpp \
    androidgdbserverkitinformation.cpp \
    androidanalyzesupport.cpp \
    androidmanifesteditorfactory.cpp \
    androidmanifesteditor.cpp \
    androidmanifesteditorwidget.cpp \
    androidmanifestdocument.cpp \
    androiddevicedialog.cpp \
    androiddeployqtstep.cpp \
    certificatesmodel.cpp \
    androiddeployqtwidget.cpp \
    androidpotentialkit.cpp \
    androidsignaloperation.cpp \
    javaeditor.cpp \
    javaindenter.cpp \
    avddialog.cpp \
    androidbuildapkstep.cpp \
    androidbuildapkwidget.cpp \
    androidqtsupport.cpp \
    androidrunnable.cpp \
    androidtoolmanager.cpp \
    androidsdkmanager.cpp \
    androidavdmanager.cpp \
    androidrunconfigurationwidget.cpp \
    adbcommandswidget.cpp \
    androidsdkpackage.cpp \
    androidsdkmodel.cpp \
    androidsdkmanagerwidget.cpp

FORMS += \
    androidsettingswidget.ui \
    addnewavddialog.ui \
    androidcreatekeystorecertificate.ui \
    androiddevicedialog.ui \
    androiddeployqtwidget.ui \
    androidbuildapkwidget.ui \
    androidrunconfigurationwidget.ui \
    adbcommandswidget.ui \
    androidsdkmanagerwidget.ui

RESOURCES = android.qrc

DEFINES += ANDROID_LIBRARY
