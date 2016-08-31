import qbs

QtcProduct {
    name: "componentsplugin"
    type: ["dynamiclibrary"]
    installDir: qtc.ide_plugin_path + '/' + installDirName
    property string installDirName: qbs.targetOS.contains("macos") ? "QmlDesigner" : "qmldesigner"

    Depends { name: "Core" }
    Depends { name: "QmlDesigner" }
    Depends { name: "Utils" }
    Depends { name: "Qt.qml" }

    cpp.defines: base.concat("COMPONENTS_LIBRARY")
    cpp.includePaths: base.concat([
        "..",
        "../components/componentcore",
        "../components/debugview",
        "../components/formeditor",
        "../components/importmanager",
        "../components/integration",
        "../components/itemlibrary",
        "../components/navigator",
        "../components/propertyeditor",
        "../components/stateseditor",
        "../designercore",
        "../designercore/include",
        "../../../../share/qtcreator/qml/qmlpuppet/interfaces",
        "../../../../share/qtcreator/qml/qmlpuppet/types",
    ])
    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.internalVersion: ""
    }

    Group {
        name: "controls"
        files: ["Controls/*.qml"]
    }

    Group {
        name: "images"
        files: ["images/*.png"]
    }

    Group {
        name: "plugin metadata"
        files: ["componentsplugin.json"]
        fileTags: ["qt_plugin_metadata"]
    }

    files: [
        "addtabdesigneraction.cpp",
        "addtabdesigneraction.h",
        "addtabtotabviewdialog.ui",
        "addtabtotabviewdialog.cpp",
        "addtabtotabviewdialog.h",
        "components.metainfo",
        "componentsplugin.cpp",
        "componentsplugin.h",
        "componentsplugin.qrc",
        "entertabdesigneraction.cpp",
        "entertabdesigneraction.h",
        "tabviewindexmodel.cpp",
        "tabviewindexmodel.h",
        "../designercore/include/iwidgetplugin.h",
    ]
}
