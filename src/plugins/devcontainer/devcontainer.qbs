QtcPlugin {
    name: "DevContainerPlugin"

    Depends { name: "Core" }
    Depends { name: "DevContainer" }
    Depends { name: "CmdBridgeClient" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "Utils" }
    Depends { name: "qtc" }

    pluginTestDepends: [
        "CMakeProjectManager"
    ]

    files: [
        "devcontainerdevice.cpp",
        "devcontainerdevice.h",
        "devcontainerplugin.cpp",
        "devcontainerplugin_constants.h",
        "devcontainerplugin_global.h",
        "devcontainerplugintr.h",
    ]

    QtcTestFiles {
        files: [
            "devcontainer_test.cpp",
        ]
        cpp.defines: outer.concat([ 'TESTDATA="' + qtc.ide_data_path + '/devcontainer_testdata"' ])
    }

    Group {
        name: "testdata"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path + "/devcontainer_testdata"
        qbs.installSourceBase: "testdata/"
        prefix: "testdata/"
        fileTags: []
        files: [
            "**/*",
            ".**/*",
        ]
    }

    Group {
        name: "images"
        prefix: "images/"
        files: [
            "container.png",
            "container@2x.png",
        ]
        fileTags: "qt.core.resource_data"
        Qt.core.resourcePrefix: "/devcontainer"
    }
}
