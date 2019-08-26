include(../../qtcreatorplugin.pri)

isEmpty(ANDROID_ENABLE):ANDROID_EXPERIMENTAL_STR="true"
else:ANDROID_EXPERIMENTAL_STR="false"

QT += xml network

HEADERS += \
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
    androidrunnerworker.h \
    androiddebugsupport.h \
    androidqtversion.h \
    androidcreatekeystorecertificate.h \
    javaparser.h \
    androidplugin.h \
    androiddevice.h \
    androidqmltoolingsupport.h \
    androidmanifesteditorfactory.h \
    androidmanifesteditor.h \
    androidmanifesteditorwidget.h \
    androidmanifestdocument.h \
    androiddevicedialog.h \
    androiddeployqtstep.h \
    certificatesmodel.h \
    androidpotentialkit.h \
    androidsignaloperation.h \
    javaeditor.h \
    javaindenter.h \
    avddialog.h \
    android_global.h \
    androidbuildapkstep.h \
    androidbuildapkwidget.h \
    androidtoolmanager.h \
    androidsdkmanager.h \
    androidavdmanager.h \
    adbcommandswidget.h \
    androidsdkpackage.h \
    androidsdkmodel.h \
    androidsdkmanagerwidget.h \
    androidpackageinstallationstep.h \
    androidextralibrarylistmodel.h \
    createandroidmanifestwizard.h

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
    androidrunnerworker.cpp \
    androiddebugsupport.cpp \
    androidqtversion.cpp \
    androidcreatekeystorecertificate.cpp \
    javaparser.cpp \
    androidplugin.cpp \
    androiddevice.cpp \
    androidqmltoolingsupport.cpp \
    androidmanifesteditorfactory.cpp \
    androidmanifesteditor.cpp \
    androidmanifesteditorwidget.cpp \
    androidmanifestdocument.cpp \
    androiddevicedialog.cpp \
    androiddeployqtstep.cpp \
    certificatesmodel.cpp \
    androidpotentialkit.cpp \
    androidsignaloperation.cpp \
    javaeditor.cpp \
    javaindenter.cpp \
    avddialog.cpp \
    androidbuildapkstep.cpp \
    androidbuildapkwidget.cpp \
    androidtoolmanager.cpp \
    androidsdkmanager.cpp \
    androidavdmanager.cpp \
    adbcommandswidget.cpp \
    androidsdkpackage.cpp \
    androidsdkmodel.cpp \
    androidsdkmanagerwidget.cpp \
    androidpackageinstallationstep.cpp \
    androidextralibrarylistmodel.cpp \
    createandroidmanifestwizard.cpp

FORMS += \
    androidsettingswidget.ui \
    addnewavddialog.ui \
    androidcreatekeystorecertificate.ui \
    androiddevicedialog.ui \
    adbcommandswidget.ui \
    androidsdkmanagerwidget.ui

RESOURCES = android.qrc

DEFINES += ANDROID_LIBRARY
