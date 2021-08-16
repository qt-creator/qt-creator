include(../../qtcreatorplugin.pri)

isEmpty(ANDROID_ENABLE):ANDROID_EXPERIMENTAL_STR="true"
else:ANDROID_EXPERIMENTAL_STR="false"

QT += xml network

HEADERS += \
    androidconstants.h \
    androidconfigurations.h \
    androidmanager.h \
    androidmanifesteditoriconcontainerwidget.h \
    androidmanifesteditoriconwidget.h \
    androidqmlpreviewworker.h \
    androidrunconfiguration.h \
    androidruncontrol.h \
    androidservicewidget.h \
    androidservicewidget_p.h \
    androidsettingswidget.h \
    androidtoolchain.h \
    androiderrormessage.h \
    androidglobal.h \
    androidrunner.h \
    androidrunnerworker.h \
    androiddebugsupport.h \
    androidqtversion.h \
    androidcreatekeystorecertificate.h \
    javaparser.h \
    androidplugin.h \
    androiddevice.h \
    androiddeviceinfo.h \
    androidqmltoolingsupport.h \
    androidmanifesteditorfactory.h \
    androidmanifesteditor.h \
    androidmanifesteditorwidget.h \
    androidmanifestdocument.h \
    androiddeployqtstep.h \
    certificatesmodel.h \
    androidpotentialkit.h \
    androidsignaloperation.h \
    javaeditor.h \
    javaindenter.h \
    avddialog.h \
    avdmanageroutputparser.h \
    android_global.h \
    androidbuildapkstep.h \
    androidsdkmanager.h \
    androidavdmanager.h \
    androidsdkpackage.h \
    androidsdkmodel.h \
    androidsdkmanagerwidget.h \
    androidpackageinstallationstep.h \
    androidextralibrarylistmodel.h \
    createandroidmanifestwizard.h \
    androidsdkdownloader.h \
    splashscreencontainerwidget.h \
    splashscreenwidget.h \
    javalanguageserver.h \

SOURCES += \
    androidconfigurations.cpp \
    androidmanager.cpp \
    androidmanifesteditoriconcontainerwidget.cpp \
    androidmanifesteditoriconwidget.cpp \
    androidqmlpreviewworker.cpp \
    androidrunconfiguration.cpp \
    androidruncontrol.cpp \
    androidservicewidget.cpp \
    androidsettingswidget.cpp \
    androidtoolchain.cpp \
    androiderrormessage.cpp \
    androidrunner.cpp \
    androidrunnerworker.cpp \
    androiddebugsupport.cpp \
    androidqtversion.cpp \
    androidcreatekeystorecertificate.cpp \
    javaparser.cpp \
    androidplugin.cpp \
    androiddevice.cpp \
    androiddeviceinfo.cpp \
    androidqmltoolingsupport.cpp \
    androidmanifesteditorfactory.cpp \
    androidmanifesteditor.cpp \
    androidmanifesteditorwidget.cpp \
    androidmanifestdocument.cpp \
    androiddeployqtstep.cpp \
    certificatesmodel.cpp \
    androidpotentialkit.cpp \
    androidsignaloperation.cpp \
    javaeditor.cpp \
    javaindenter.cpp \
    avddialog.cpp \
    avdmanageroutputparser.cpp \
    androidbuildapkstep.cpp \
    androidsdkmanager.cpp \
    androidavdmanager.cpp \
    androidsdkpackage.cpp \
    androidsdkmodel.cpp \
    androidsdkmanagerwidget.cpp \
    androidpackageinstallationstep.cpp \
    androidextralibrarylistmodel.cpp \
    createandroidmanifestwizard.cpp \
    androidsdkdownloader.cpp \
    splashscreencontainerwidget.cpp \
    splashscreenwidget.cpp \
    javalanguageserver.cpp \

FORMS += \
    androidsettingswidget.ui \
    addnewavddialog.ui \
    androidcreatekeystorecertificate.ui \
    androidsdkmanagerwidget.ui

RESOURCES = android.qrc

DEFINES += ANDROID_LIBRARY
