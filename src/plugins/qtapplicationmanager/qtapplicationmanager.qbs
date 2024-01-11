import qbs 1.0

QtcPlugin {
    name: "QtApplicationManagerIntegration"

    Depends { name: "Boot2Qt"; required: false }
    Depends { name: "Core" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "QmlJS" }
    Depends { name: "QtSupport" }
    Depends { name: "RemoteLinux" }
    Depends { name: "ResourceEditor" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "yaml-cpp" }

    //Properties {
    //    condition: Boot2Qt.present
    //    cpp.defines: base.concat("HAVE_BOOT2QT")
    //

    files: [
        "appmanagerconstants.h",
        "appmanagercmakepackagestep.cpp",
        "appmanagercmakepackagestep.h",
        "appmanagercreatepackagestep.cpp",
        "appmanagercreatepackagestep.h",
        "appmanagerdeployconfigurationautoswitcher.cpp",
        "appmanagerdeployconfigurationautoswitcher.h",
        "appmanagerdeployconfigurationfactory.cpp",
        "appmanagerdeployconfigurationfactory.h",
        "appmanagerdeploypackagestep.cpp",
        "appmanagerdeploypackagestep.h",
        "appmanagerinstallpackagestep.cpp",
        "appmanagerinstallpackagestep.h",
        "appmanagermakeinstallstep.cpp",
        "appmanagermakeinstallstep.h",
        "appmanagerplugin.cpp",
        "appmanagerremoteinstallpackagestep.cpp",
        "appmanagerremoteinstallpackagestep.h",
        "appmanagerrunconfiguration.cpp",
        "appmanagerrunconfiguration.h",
        "appmanagerruncontrol.cpp",
        "appmanagerruncontrol.h",
        "appmanagerstringaspect.cpp",
        "appmanagerstringaspect.h",
        "appmanagertargetinformation.cpp",
        "appmanagertargetinformation.h",
        "appmanagertr.h",
        "appmanagerutilities.cpp",
        "appmanagerutilities.h",
    ]
}

