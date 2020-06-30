import qbs 1.0

Project {
    name: "Android"

    QtcDevHeaders { }

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
        Depends { name: "Core" }
        Depends { name: "Debugger" }
        Depends { name: "ProParser" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QmlDebug" }
        Depends { name: "QtSupport" }
        Depends { name: "TextEditor" }
        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "android_global.h",
            "android.qrc",
            "adbcommandswidget.cpp",
            "adbcommandswidget.h",
            "adbcommandswidget.ui",
            "addnewavddialog.ui",
            "androidavdmanager.cpp",
            "androidavdmanager.h",
            "androidconfigurations.cpp",
            "androidconfigurations.h",
            "androidconstants.h",
            "androidcreatekeystorecertificate.cpp",
            "androidcreatekeystorecertificate.h",
            "androidcreatekeystorecertificate.ui",
            "androidbuildapkstep.cpp",
            "androidbuildapkstep.h",
            "androidbuildapkwidget.cpp",
            "androidbuildapkwidget.h",
            "androiddeployqtstep.cpp",
            "androiddeployqtstep.h",
            "androiddebugsupport.cpp",
            "androiddebugsupport.h",
            "androiddevicedialog.cpp",
            "androiddevicedialog.h",
            "androiddevicedialog.ui",
            "androiddevice.cpp",
            "androiddevice.h",
            "androiderrormessage.h",
            "androiderrormessage.cpp",
            "androidextralibrarylistmodel.cpp",
            "androidextralibrarylistmodel.h",
            "androidglobal.h",
            "androidmanager.cpp",
            "androidmanager.h",
            "androidmanifestdocument.cpp",
            "androidmanifestdocument.h",
            "androidmanifesteditor.cpp",
            "androidmanifesteditor.h",
            "androidmanifesteditoriconwidget.cpp",
            "androidmanifesteditoriconwidget.h",
            "androidmanifesteditoriconcontainerwidget.cpp",
            "androidmanifesteditoriconcontainerwidget.h",
            "androidmanifesteditorfactory.cpp",
            "androidmanifesteditorfactory.h",
            "androidmanifesteditorwidget.cpp",
            "androidmanifesteditorwidget.h",
            "androidpackageinstallationstep.cpp",
            "androidpackageinstallationstep.h",
            "androidplugin.cpp",
            "androidplugin.h",
            "androidpotentialkit.cpp",
            "androidpotentialkit.h",
            "androidqmltoolingsupport.cpp",
            "androidqmltoolingsupport.h",
            "androidqtversion.cpp",
            "androidqtversion.h",
            "androidrunconfiguration.cpp",
            "androidrunconfiguration.h",
            "androidruncontrol.cpp",
            "androidruncontrol.h",
            "androidrunner.cpp",
            "androidrunner.h",
            "androidrunnerworker.cpp",
            "androidrunnerworker.h",
            "androidsdkdownloader.cpp",
            "androidsdkdownloader.h",
            "androidsdkmanager.cpp",
            "androidsdkmanager.h",
            "androidsdkmanagerwidget.cpp",
            "androidsdkmanagerwidget.h",
            "androidsdkmanagerwidget.ui",
            "androidsdkmodel.cpp",
            "androidsdkmodel.h",
            "androidsdkpackage.cpp",
            "androidsdkpackage.h",
            "androidservicewidget.cpp",
            "androidservicewidget.h",
            "androidservicewidget_p.h",
            "androidsettingswidget.cpp",
            "androidsettingswidget.h",
            "androidsettingswidget.ui",
            "androidsignaloperation.cpp",
            "androidsignaloperation.h",
            "androidtoolchain.cpp",
            "androidtoolchain.h",
            "avddialog.cpp",
            "avddialog.h",
            "certificatesmodel.cpp",
            "certificatesmodel.h",
            "createandroidmanifestwizard.h",
            "createandroidmanifestwizard.cpp",
            "javaeditor.cpp",
            "javaeditor.h",
            "javaindenter.cpp",
            "javaindenter.h",
            "javaparser.cpp",
            "javaparser.h",
            "splashiconcontainerwidget.cpp",
            "splashiconcontainerwidget.h"
        ]
    }
}
