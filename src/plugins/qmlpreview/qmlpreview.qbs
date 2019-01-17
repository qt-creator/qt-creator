import qbs

QtcPlugin {
    name: "QmlPreview"

    Depends { name: "Core" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlDebug" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "QtSupport" }
    Depends { name: "ResourceEditor" }
    Depends { name: "Utils" }

    Depends {
        name: "Qt"
        submodules: ["core"]
    }

    Group {
        name: "General"
        files: [
            "qmlpreviewclient.cpp",
            "qmlpreviewclient.h",
            "qmlpreviewconnectionmanager.cpp",
            "qmlpreviewconnectionmanager.h",
            "qmlpreviewfileontargetfinder.cpp",
            "qmlpreviewfileontargetfinder.h",
            "qmlpreview_global.h",
            "qmlpreviewplugin.cpp",
            "qmlpreviewplugin.h",
            "qmlpreviewruncontrol.cpp",
            "qmlpreviewruncontrol.h",
        ]
    }

    Group {
        name: "Unit tests"
        condition: qtc.testsEnabled
        prefix: "tests/"
        files: [
            "qmlpreviewclient_test.cpp",
            "qmlpreviewclient_test.h",
            "qmlpreviewplugin_test.cpp",
            "qmlpreviewplugin_test.h",
        ]
    }
}
