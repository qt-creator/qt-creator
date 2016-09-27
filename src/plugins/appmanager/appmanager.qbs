import qbs 1.0

QtcPlugin {
    name: "AppManager"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlJS" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["widgets", "network"] }

    files: [
        "appmanager.qrc",
        "appmanagerconstants.h",
        "appmanagerplugin.h",
        "appmanagerplugin.cpp",
        "project/appmanagerproject.cpp",
        "project/appmanagerproject.h",
        "project/appmanagerprojectmanager.cpp",
        "project/appmanagerprojectmanager.h",
        "project/appmanagerprojectnode.cpp",
        "project/appmanagerprojectnode.h",
        "project/appmanagerrunconfiguration.cpp",
        "project/appmanagerrunconfiguration.h",
        "project/appmanagerrunconfigurationfactory.cpp",
        "project/appmanagerrunconfigurationfactory.h",
        "project/appmanagerruncontrol.cpp",
        "project/appmanagerruncontrol.h",
        "project/appmanagerruncontrolfactory.cpp",
        "project/appmanagerruncontrolfactory.h",
    ]
}

