add_qtc_plugin(Android
  DEPENDS QtcSsh QmlDebug Qt5::Xml
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport
  SOURCES
    adbcommandswidget.cpp adbcommandswidget.h adbcommandswidget.ui
    addnewavddialog.ui
    android.qrc
    android_global.h
    androidavdmanager.cpp androidavdmanager.h
    androidbuildapkstep.cpp androidbuildapkstep.h
    androidbuildapkwidget.cpp androidbuildapkwidget.h
    androidconfigurations.cpp androidconfigurations.h
    androidconstants.h
    androidcreatekeystorecertificate.cpp androidcreatekeystorecertificate.h androidcreatekeystorecertificate.ui
    androiddebugsupport.cpp androiddebugsupport.h
    androiddeployqtstep.cpp androiddeployqtstep.h
    androiddevice.cpp androiddevice.h
    androiddevicedialog.cpp androiddevicedialog.h androiddevicedialog.ui
    androiderrormessage.cpp androiderrormessage.h
    androidextralibrarylistmodel.cpp androidextralibrarylistmodel.h
    androidglobal.h
    androidmanager.cpp androidmanager.h
    androidmanifestdocument.cpp androidmanifestdocument.h
    androidmanifesteditor.cpp androidmanifesteditor.h
    androidmanifesteditorfactory.cpp androidmanifesteditorfactory.h
    androidmanifesteditorwidget.cpp androidmanifesteditorwidget.h
    androidmanifesteditoriconwidget.cpp androidmanifesteditoriconwidget.h
    androidmanifesteditoriconcontainerwidget.cpp androidmanifesteditoriconcontainerwidget.h
    androidpackageinstallationstep.cpp androidpackageinstallationstep.h
    androidplugin.cpp androidplugin.h
    androidpotentialkit.cpp androidpotentialkit.h
    androidqmltoolingsupport.cpp androidqmltoolingsupport.h
    androidqtversion.cpp androidqtversion.h
    androidrunconfiguration.cpp androidrunconfiguration.h
    androidruncontrol.cpp androidruncontrol.h
    androidrunner.cpp androidrunner.h
    androidrunnerworker.cpp androidrunnerworker.h
    androidsdkdownloader.cpp androidsdkdownloader.h
    androidsdkmanager.cpp androidsdkmanager.h
    androidsdkmanagerwidget.cpp androidsdkmanagerwidget.h androidsdkmanagerwidget.ui
    androidsdkmodel.cpp androidsdkmodel.h
    androidsdkpackage.cpp androidsdkpackage.h
    androidservicewidget.cpp androidservicewidget.h androidservicewidget_p.h
    androidsettingswidget.cpp androidsettingswidget.h androidsettingswidget.ui
    androidsignaloperation.cpp androidsignaloperation.h
    androidtoolchain.cpp androidtoolchain.h
    avddialog.cpp avddialog.h
    certificatesmodel.cpp certificatesmodel.h
    createandroidmanifestwizard.cpp createandroidmanifestwizard.h
    javaeditor.cpp javaeditor.h
    javaindenter.cpp javaindenter.h
    javaparser.cpp javaparser.h
    splashiconcontainerwidget.cpp splashiconcontainerwidget.h
)
