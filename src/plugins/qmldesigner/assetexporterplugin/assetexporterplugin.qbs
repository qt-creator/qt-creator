import qbs

QtcProduct {
    name: "assetexporterplugin"
    type: ["dynamiclibrary"]
    installDir: qtc.ide_plugin_path + '/' + installDirName
    property string installDirName: qbs.targetOS.contains("macos") ? "QmlDesigner" : "qmldesigner"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlDesigner" }
    Depends { name: "Utils" }
    Depends {
        name: "Qt"
        submodules: [
            "quick-private"
        ]
    }

    cpp.includePaths: base.concat([
        "./",
        "../designercore/include",
        "../../../../share/qtcreator/qml/qmlpuppet/interfaces",
        "../../../../share/qtcreator/qml/qmlpuppet/types"
    ])

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.internalVersion: ""
    }

    Group {
        name: "plugin metadata"
        files: ["assetexporterplugin.json"]
        fileTags: ["qt_plugin_metadata"]
    }

    files: [
        "assetexportdialog.cpp",
        "assetexportdialog.h",
        "assetexportdialog.ui",
        "assetexporter.cpp",
        "assetexporter.h",
        "assetexporterplugin.cpp",
        "assetexporterplugin.h",
        "assetexporterplugin.qrc",
        "assetexporterview.cpp",
        "assetexporterview.h",
        "assetexportpluginconstants.h",
        "componentexporter.cpp",
        "componentexporter.h",
        "exportnotification.cpp",
        "exportnotification.h",
        "filepathmodel.cpp",
        "filepathmodel.h",
        "dumpers/assetnodedumper.cpp",
        "dumpers/assetnodedumper.h",
        "dumpers/itemnodedumper.cpp",
        "dumpers/itemnodedumper.h",
        "dumpers/nodedumper.cpp",
        "dumpers/nodedumper.h",
        "dumpers/textnodedumper.cpp",
        "dumpers/textnodedumper.h"
    ]
}
