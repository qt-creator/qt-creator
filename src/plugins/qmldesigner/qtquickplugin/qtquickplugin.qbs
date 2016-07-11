import qbs

QtcProduct {
    name: "qtquickplugin"
    type: ["dynamiclibrary"]
    installDir: qtc.ide_plugin_path + '/' + installDirName
    property string installDirName: qbs.targetOS.contains("osx") ? "QmlDesigner" : "qmldesigner"

    cpp.defines: base.concat("QTQUICK_LIBRARY")
    cpp.includePaths: base.concat("../designercore/include")
    cpp.internalVersion: ""

    Group {
        name: "sources"
        files: ["sources/*.qml"]
    }

    Group {
        name: "images"
        files: ["images/*.png"]
    }

    Group {
        name: "plugin metadata"
        files: ["qtquickplugin.json"]
        fileTags: ["qt_plugin_metadata"]
    }

    files: [
        "quick.metainfo",
        "qtquickplugin.cpp",
        "qtquickplugin.h",
        "qtquickplugin.qrc",
        "../designercore/include/iwidgetplugin.h",
    ]
}
