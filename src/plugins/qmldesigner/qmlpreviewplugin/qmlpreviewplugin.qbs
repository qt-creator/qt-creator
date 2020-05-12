import qbs

QtcProduct {
    name: "qmlpreviewplugin"
    type: ["dynamiclibrary"]
    installDir: qtc.ide_plugin_path + '/' + installDirName
    property string installDirName: qbs.targetOS.contains("macos") ? "QmlDesigner" : "qmldesigner"

    cpp.defines: base.concat("QMLPREVIEW_LIBRARY")
    cpp.includePaths: base.concat("../designercore/include")
    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.internalVersion: ""
    }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlDesigner" }
    Depends { name: "Qt.qml" }
    Depends { name: "Utils" }

    Group {
        name: "images"
        files: ["images/*.png"]
    }

    Group {
        name: "plugin metadata"
        files: ["qmlpreviewplugin.json"]
        fileTags: ["qt_plugin_metadata"]
    }

    files: [
        "qmlpreviewactions.cpp",
        "qmlpreviewactions.h",
        "qmlpreviewplugin.cpp",
        "qmlpreviewplugin.h",
        "qmlpreviewplugin.qrc",
    ]
}
