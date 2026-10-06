QtcPlugin {
    name: "Remote"

    Depends { name: "Qt.widgets" }

    Depends { name: "CmdBridgeClient" }
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
        "connectdevicestep.cpp",
        "connectdevicestep.h",
        "customcommanddeploystep.cpp",
        "customcommanddeploystep.h",
        "genericdeploystep.cpp",
        "genericdeploystep.h",
        "genericdirectuploadstep.cpp",
        "genericdirectuploadstep.h",
        "killappstep.cpp",
        "killappstep.h",
        "linuxdevice.cpp",
        "linuxdevice.h",
        "linuxdevicetester.cpp",
        "linuxdevicetester.h",
        "linuxprocessinterface.h",
        "macdevice.cpp",
        "macdevice.h",
        "makeinstallstep.cpp",
        "makeinstallstep.h",
        "powershellutils.h",
        "publickeydeploymentdialog.cpp",
        "publickeydeploymentdialog.h",
        "remotelinux_constants.h",
        "remotelinux_export.h",
        "remotelinuxcustomrunconfiguration.cpp",
        "remotelinuxcustomrunconfiguration.h",
        "remotelinuxdebugsupport.cpp",
        "remotelinuxdebugsupport.h",
        "remotelinuxdeploysupport.cpp",
        "remotelinuxdeploysupport.h",
        "remotelinuxenvironmentaspect.cpp",
        "remotelinuxenvironmentaspect.h",
        "remotelinuxfiletransfer.cpp",
        "remotelinuxfiletransfer.h",
        "remotelinuxplugin.cpp",
        "remotelinuxrunconfiguration.cpp",
        "remotelinuxrunconfiguration.h",
        "remotelinuxtr.h",
        "sshconnectionsharing.cpp",
        "sshconnectionsharing.h",
        "sshdevicewizard.cpp",
        "sshdevicewizard.h",
        "sshkeycreationdialog.cpp",
        "sshkeycreationdialog.h",
        "tarpackagecreationstep.cpp",
        "tarpackagecreationstep.h",
        "tarpackagedeploystep.cpp",
        "tarpackagedeploystep.h",
        "windowsdevice.cpp",
        "windowsdevice.h",
        "windowsdevicetester.cpp",
        "windowsdevicetester.h",
    ]

    QtcTestFiles {
        files: [
            "filesystemaccess_test.cpp",
            "filesystemaccess_test.h",
            "remoterun_test.cpp",
            "remoterun_test.h",
            "windowsdevicedetection_test.cpp",
            "windowsdevicedetection_test.h",
        ]
    }

    Group {
        name: "images"
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/remotelinux"
        files: [
            "images/embeddedtarget.png",
            "images/macosdevice.png",
            "images/macosdevice@2x.png",
            "images/macosdevicesmall.png",
            "images/macosdevicesmall@2x.png",
            "images/windowsdevice.png",
            "images/windowsdevice@2x.png",
            "images/windowsdevicesmall.png",
            "images/windowsdevicesmall@2x.png",
            "images/linuxdevice.png",
            "images/linuxdevice@2x.png",
            "images/linuxdevicesmall.png",
            "images/linuxdevicesmall@2x.png",
        ]
    }

    Export {
        Depends { name: "Debugger" }
        Depends { name: "Core" }
    }
}
