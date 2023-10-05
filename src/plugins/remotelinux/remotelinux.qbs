QtcPlugin {
    name: "RemoteLinux"

    Depends { name: "Qt.widgets" }
    Depends { name: "QmlDebug" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }

    files: [
        "abstractremotelinuxdeploystep.cpp",
        "abstractremotelinuxdeploystep.h",
        "deploymenttimeinfo.cpp",
        "deploymenttimeinfo.h",
        "customcommanddeploystep.cpp",
        "customcommanddeploystep.h",
        "genericdeploystep.cpp",
        "genericdeploystep.h",
        "genericdirectuploadstep.cpp",
        "genericdirectuploadstep.h",
        "genericlinuxdeviceconfigurationwidget.cpp",
        "genericlinuxdeviceconfigurationwidget.h",
        "killappstep.cpp",
        "killappstep.h",
        "linuxdevice.cpp",
        "linuxdevice.h",
        "linuxdevicetester.cpp",
        "linuxdevicetester.h",
        "linuxprocessinterface.h",
        "makeinstallstep.cpp",
        "makeinstallstep.h",
        "publickeydeploymentdialog.cpp",
        "publickeydeploymentdialog.h",
        "remotelinux.qrc",
        "remotelinux_constants.h",
        "remotelinux_export.h",
        "remotelinuxcustomrunconfiguration.cpp",
        "remotelinuxcustomrunconfiguration.h",
        "remotelinuxdebugsupport.cpp",
        "remotelinuxdebugsupport.h",
        "remotelinuxenvironmentaspect.cpp",
        "remotelinuxenvironmentaspect.h",
        "remotelinuxplugin.cpp",
        "remotelinuxplugin.h",
        "remotelinuxrunconfiguration.cpp",
        "remotelinuxrunconfiguration.h",
        "remotelinuxsignaloperation.cpp",
        "remotelinuxsignaloperation.h",
        "remotelinuxtr.h",
        "sshdevicewizard.cpp",
        "sshdevicewizard.h",
        "sshkeycreationdialog.cpp",
        "sshkeycreationdialog.h",
        "tarpackagecreationstep.cpp",
        "tarpackagecreationstep.h",
        "tarpackagedeploystep.cpp",
        "tarpackagedeploystep.h",
        "images/embeddedtarget.png",
    ]

    QtcTestFiles {
        files: [
            "filesystemaccess_test.cpp",
            "filesystemaccess_test.h",
        ]
    }

    Export {
        Depends { name: "Debugger" }
        Depends { name: "Core" }
    }
}
