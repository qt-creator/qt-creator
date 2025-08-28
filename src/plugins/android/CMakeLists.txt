add_qtc_plugin(Android
  DEPENDS QmlDebug Qt::Xml LanguageServerProtocol
  PLUGIN_DEPENDS Core Debugger ProjectExplorer QtSupport LanguageClient
  LONG_DESCRIPTION_MD plugindescription.md
  LICENSE_MD LICENSE.md
  SOURCES
    androidtr.h
    androidbuildapkstep.cpp androidbuildapkstep.h
    androidconfigurations.cpp androidconfigurations.h
    androidconstants.h
    androiddebugsupport.cpp androiddebugsupport.h
    androiddeployqtstep.cpp androiddeployqtstep.h
    androiddevice.cpp androiddevice.h
    androiddeviceinfo.cpp androiddeviceinfo.h
    androidmanifesteditor.cpp androidmanifesteditor.h
    androidpackageinstallationstep.cpp androidpackageinstallationstep.h
    androidplugin.cpp
    androidqmltoolingsupport.cpp androidqmltoolingsupport.h
    androidqtversion.cpp androidqtversion.h
    androidrunconfiguration.cpp androidrunconfiguration.h
    androidrunner.cpp androidrunner.h
    androidrunnerworker.cpp androidrunnerworker.h
    androidsdkdownloader.cpp androidsdkdownloader.h
    androidsdkmanager.cpp androidsdkmanager.h
    androidsdkmanagerdialog.cpp androidsdkmanagerdialog.h
    androidsdkpackage.cpp androidsdkpackage.h
    androidsettingswidget.cpp androidsettingswidget.h
    androidsignaloperation.cpp androidsignaloperation.h
    androidtoolchain.cpp androidtoolchain.h
    androidutils.cpp androidutils.h
    avdcreatordialog.cpp avdcreatordialog.h
    avdmanageroutputparser.cpp avdmanageroutputparser.h
    javaeditor.cpp javaeditor.h
    javalanguageserver.cpp javalanguageserver.h
    javaparser.cpp javaparser.h
    keystorecertificatedialog.cpp keystorecertificatedialog.h
    manifestwizard.cpp manifestwizard.h
    splashscreencontainerwidget.cpp splashscreencontainerwidget.h
    sdkmanageroutputparser.cpp sdkmanageroutputparser.h
)

extend_qtc_plugin(Android
  CONDITION WITH_TESTS
  SOURCES
    androidsdkmanager_test.cpp androidsdkmanager_test.h
    sdkmanageroutputparser_test.cpp sdkmanageroutputparser_test.h
    android_tst.qrc
)

qtc_add_resources(Android "images"
  PREFIX "/android"
  BASE "."
  FILES
    images/androiddevice.png
    images/androiddevice@2x.png
    images/androiddevicesmall.png
    images/androiddevicesmall@2x.png
)
