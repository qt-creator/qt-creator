QtcPlugin {
    name: "Android"

    Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "LanguageClient" }
    Depends { name: "LanguageServerProtocol" }
    Depends { name: "ProParser" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlDebug" }
    Depends { name: "QtSupport" }
    Depends { name: "Spinner" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }

    files: [
        "androidtr.h",
        "androidconfigurations.cpp",
        "androidconfigurations.h",
        "androidconstants.h",
        "androidbuildapkstep.cpp",
        "androidbuildapkstep.h",
        "androiddeployqtstep.cpp",
        "androiddeployqtstep.h",
        "androiddebugsupport.cpp",
        "androiddebugsupport.h",
        "androiddevice.cpp",
        "androiddevice.h",
        "androiddeviceinfo.cpp",
        "androiddeviceinfo.h",
        "androidmanifesteditor.cpp",
        "androidmanifesteditor.h",
        "androidpackageinstallationstep.cpp",
        "androidpackageinstallationstep.h",
        "androidplugin.cpp",
        "androidqmltoolingsupport.cpp",
        "androidqmltoolingsupport.h",
        "androidqtversion.cpp",
        "androidqtversion.h",
        "androidrunconfiguration.cpp",
        "androidrunconfiguration.h",
        "androidrunner.cpp",
        "androidrunner.h",
        "androidrunnerworker.cpp",
        "androidrunnerworker.h",
        "androidsdkdownloader.cpp",
        "androidsdkdownloader.h",
        "androidsdkmanager.cpp",
        "androidsdkmanager.h",
        "androidsdkmanagerdialog.cpp",
        "androidsdkmanagerdialog.h",
        "androidsdkpackage.cpp",
        "androidsdkpackage.h",
        "androidsettingswidget.cpp",
        "androidsettingswidget.h",
        "androidsignaloperation.cpp",
        "androidsignaloperation.h",
        "androidtoolchain.cpp",
        "androidtoolchain.h",
        "androidutils.cpp",
        "androidutils.h",
        "avdcreatordialog.cpp",
        "avdcreatordialog.h",
        "avdmanageroutputparser.cpp",
        "avdmanageroutputparser.h",
        "javaeditor.cpp",
        "javaeditor.h",
        "javalanguageserver.cpp",
        "javalanguageserver.h",
        "javaparser.cpp",
        "javaparser.h",
        "keystorecertificatedialog.cpp",
        "keystorecertificatedialog.h",
        "manifestwizard.h",
        "manifestwizard.cpp",
        "splashscreencontainerwidget.cpp",
        "splashscreencontainerwidget.h",
        "sdkmanageroutputparser.cpp",
        "sdkmanageroutputparser.h"
    ]

    Group {
        name: "license"
        files: "LICENSE.md"
        fileTags: "pluginjson.license"
    }

    Group {
        name: "long description"
        files: "plugindescription.md"
        fileTags: "pluginjson.longDescription"
    }

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "androiddevice.png",
            "androiddevice@2x.png",
            "androiddevicesmall.png",
            "androiddevicesmall@2x.png",
        ]
        fileTags: "qt.core.resource_data"
    }

    QtcTestFiles {
        files: [
            "android_tst.qrc",
            "androidsdkmanager_test.cpp",
            "androidsdkmanager_test.h",
            "sdkmanageroutputparser_test.cpp",
            "sdkmanageroutputparser_test.h",
        ]
    }
}
