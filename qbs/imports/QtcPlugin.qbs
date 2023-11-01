import qbs.FileInfo
import QtcFunctions

QtcProduct {
    destinationDirectory: FileInfo.joinPaths(project.buildDirectory, qtc.ide_plugin_path)
    name: project.name
    targetName: QtcFunctions.qtLibraryName(qbs, name)
    type: ["dynamiclibrary", "pluginSpec"]

    installDir: qtc.ide_plugin_path
    installTags: ["dynamiclibrary", "debuginfo_dll"]
    useGuiPchFile: true

    property stringList pluginRecommends: []
    property stringList pluginTestDepends: []

    Depends { name: "Qt.testlib"; condition: qtc.withPluginTests }
    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginjson" }

    cpp.defines: base.concat([name.toUpperCase() + "_LIBRARY"])
    Properties { cpp.internalVersion: ""; condition: qbs.targetOS.contains("unix") }
    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("macos"))
            flags.push("-compatibility_version", qtc.qtcreator_compat_version);
        return flags;
    }
    cpp.rpaths: qbs.targetOS.contains("macos")
        ? ["@loader_path/../Frameworks", "@loader_path/../PlugIns"]
        : ["$ORIGIN", "$ORIGIN/.."]
    cpp.sonamePrefix: qbs.targetOS.contains("macos")
        ? "@rpath"
        : undefined
    pluginjson.useVcsData: false

    Group {
        name: "PluginMetaData"
        prefix: sourceDirectory + '/'
        files: product.name + ".json.in"
        fileTags: "pluginJsonIn"
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "ExtensionSystem" }
        cpp.includePaths: ".."
    }
}
