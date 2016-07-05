import qbs 1.0
import qbs.FileInfo
import QtcFunctions

QtcProduct {
    type: ["dynamiclibrary", "pluginSpec", "qtc.dev-module"]
    installDir: qtc.ide_plugin_path
    installTags: ["dynamiclibrary"]
    useGuiPchFile: true

    property var pluginJsonReplacements
    property var pluginRecommends: []
    property var pluginTestDepends: []

    property string minimumQtVersion: "5.5.0"
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, minimumQtVersion)

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: qtc.ide_plugin_path

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginjson" }
    Depends {
        condition: qtc.testsEnabled
        name: "Qt.test"
    }

    cpp.internalVersion: ""
    cpp.defines: base.concat([name.toUpperCase() + "_LIBRARY"])
    cpp.sonamePrefix: qbs.targetOS.contains("osx")
        ? "@rpath"
        : undefined
    cpp.rpaths: qbs.targetOS.contains("osx")
        ? ["@loader_path/../Frameworks", "@loader_path/../PlugIns"]
        : ["$ORIGIN", "$ORIGIN/.."]
    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("osx"))
            flags.push("-compatibility_version", qtc.qtcreator_compat_version);
        return flags;
    }

    property string pluginIncludeBase: ".." // #include <plugin/header.h>
    cpp.includePaths: [pluginIncludeBase]

    Group {
        name: "PluginMetaData"
        files: [ product.name + ".json.in" ]
        fileTags: ["pluginJsonIn"]
    }

    Group {
        name: "MimeTypes"
        files: [ "*.mimetypes.xml" ]
    }

    Export {
        Depends { name: "ExtensionSystem" }
        Depends { name: "cpp" }
        cpp.includePaths: [product.pluginIncludeBase]
    }
}
