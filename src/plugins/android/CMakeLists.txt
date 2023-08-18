add_qtc_plugin(Android
  DEPENDS QmlDebug Qt::Xml LanguageServerProtocol
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport LanguageClient
  SOURCES
    android.qrc
    androidtr.h
    androidavdmanager.cpp androidavdmanager.h
    androidbuildapkstep.cpp androidbuildapkstep.h
    androidconfigurations.cpp androidconfigurations.h
    androidconstants.h
    androidcreatekeystorecertificate.cpp androidcreatekeystorecertificate.h
    androiddebugsupport.cpp androiddebugsupport.h
    androiddeployqtstep.cpp androiddeployqtstep.h
    androiddevice.cpp androiddevice.h
    androiddeviceinfo.cpp androiddeviceinfo.h
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
    androidqmlpreviewworker.cpp androidqmlpreviewworker.h
    androidqmltoolingsupport.cpp androidqmltoolingsupport.h
    androidqtversion.cpp androidqtversion.h
    androidrunconfiguration.cpp androidrunconfiguration.h
    androidruncontrol.cpp androidruncontrol.h
    androidrunner.cpp androidrunner.h
    androidrunnerworker.cpp androidrunnerworker.h
    androidsdkdownloader.cpp androidsdkdownloader.h
    androidsdkmanager.cpp androidsdkmanager.h
    androidsdkmanagerwidget.cpp androidsdkmanagerwidget.h
    androidsdkmodel.cpp androidsdkmodel.h
    androidsdkpackage.cpp androidsdkpackage.h
    androidsettingswidget.cpp androidsettingswidget.h
    androidsignaloperation.cpp androidsignaloperation.h
    androidtoolchain.cpp androidtoolchain.h
    avddialog.cpp avddialog.h
    avdmanageroutputparser.cpp avdmanageroutputparser.h
    certificatesmodel.cpp certificatesmodel.h
    createandroidmanifestwizard.cpp createandroidmanifestwizard.h
    javaeditor.cpp javaeditor.h
    javaindenter.cpp javaindenter.h
    javalanguageserver.cpp javalanguageserver.h
    javaparser.cpp javaparser.h
    splashscreencontainerwidget.cpp splashscreencontainerwidget.h
    splashscreenwidget.cpp splashscreenwidget.h
    sdkmanageroutputparser.cpp sdkmanageroutputparser.h
)

extend_qtc_plugin(Android
  CONDITION WITH_TESTS
  SOURCES
    androidsdkmanager_test.cpp androidsdkmanager_test.h
    sdkmanageroutputparser_test.cpp sdkmanageroutputparser_test.h
    android_tst.qrc
)
