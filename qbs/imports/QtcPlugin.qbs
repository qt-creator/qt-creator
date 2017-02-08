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

    property string minimumQtVersion: "5.6.0"
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, minimumQtVersion)

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: qtc.ide_plugin_path

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginjson" }
    Depends {
        condition: qtc.testsEnabled
        name: "Qt.testlib"
    }

    Properties {
        condition: qbs.targetOS.contains("unix")
        cpp.internalVersion: ""
    }
    cpp.defines: base.concat([name.toUpperCase() + "_LIBRARY"])
    cpp.sonamePrefix: qbs.targetOS.contains("macos")
        ? "@rpath"
        : undefined
    cpp.rpaths: qbs.targetOS.contains("macos")
        ? ["@loader_path/../Frameworks", "@loader_path/../PlugIns"]
        : ["$ORIGIN", "$ORIGIN/.."]
    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("macos"))
            flags.push("-compatibility_version", qtc.qtcreator_compat_version);
        return flags;
    }

    property string pluginIncludeBase: ".." // #include <plugin/header.h>
    cpp.includePaths: [pluginIncludeBase]

    Group {
        name: "PluginMetaData"
        prefix: product.sourceDirectory + '/'
        files: [ product.name + ".json.in" ]
        fileTags: ["pluginJsonIn"]
    }

    Export {
        Depends { name: "ExtensionSystem" }
        Depends { name: "cpp" }
        cpp.includePaths: [product.pluginIncludeBase]
    }
}
