import qbs 1.0
import qbs.FileInfo
import QtcFunctions
import QtcProduct

QtcProduct {
    type: ["dynamiclibrary", "pluginSpec"]
    installDir: project.ide_plugin_path

    property var pluginspecreplacements
    property var pluginRecommends: []

    property string minimumQtVersion: "4.8"
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, minimumQtVersion)

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_plugin_path

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginspec" }
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    cpp.defines: base.concat([name.toUpperCase() + "_LIBRARY"])
    cpp.installNamePrefix: "@rpath/PlugIns/"
    cpp.rpaths: qbs.targetOS.contains("osx") ? ["@loader_path/..", "@executable_path/.."]
                                      : ["$ORIGIN", "$ORIGIN/.."]
    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        return flags;
    }

    property string pluginIncludeBase: ".." // #include <plugin/header.h>
    cpp.includePaths: [pluginIncludeBase]

    Group {
        name: "PluginSpec"
        files: [ product.name + ".pluginspec.in" ]
        fileTags: ["pluginSpecIn"]
    }

    Group {
        name: "MimeTypes"
        files: [ "*.mimetypes.xml" ]
    }

    Export {
        Depends { name: "ExtensionSystem" }
        Depends { name: "cpp" }
        cpp.includePaths: [pluginIncludeBase]
    }
}
