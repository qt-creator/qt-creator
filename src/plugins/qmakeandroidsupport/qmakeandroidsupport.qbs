import qbs

QtcPlugin {
    name: "QmakeAndroidSupport"
    Depends { name: "Android" }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmakeProjectManager" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "QtSupport" }
    Depends { name: "ResourceEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.widgets" }

    files: [
        "androidextralibrarylistmodel.cpp",
        "androidextralibrarylistmodel.h",
        "androidpackageinstallationfactory.cpp",
        "androidpackageinstallationfactory.h",
        "androidpackageinstallationstep.cpp",
        "androidpackageinstallationstep.h",
        "createandroidmanifestwizard.cpp",
        "createandroidmanifestwizard.h",
        "qmakeandroidbuildapkstep.cpp",
        "qmakeandroidbuildapkstep.h",
        "qmakeandroidbuildapkwidget.cpp",
        "qmakeandroidbuildapkwidget.h",
        "qmakeandroidbuildapkwidget.ui",
        "androidqmakebuildconfigurationfactory.cpp",
        "androidqmakebuildconfigurationfactory.h",
        "qmakeandroidrunconfiguration.cpp",
        "qmakeandroidrunconfiguration.h",
        "qmakeandroidrunfactories.cpp",
        "qmakeandroidrunfactories.h",
        "qmakeandroidsupport.cpp",
        "qmakeandroidsupport.h",
        "qmakeandroidsupportplugin.h",
        "qmakeandroidsupportplugin.cpp",
    ]
}
