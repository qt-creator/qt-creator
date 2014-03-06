import qbs.base 1.0
import qbs.FileInfo
import QtcFunctions

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    property string provider: 'QtProject'
    property var pluginspecreplacements
    property var pluginRecommends: []

    Depends { name: "Qt.core" }
    property string minimumQtVersion: "4.8"
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, minimumQtVersion)

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_plugin_path + '/' + provider

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginspec" }
    Depends { name: "cpp" }
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    cpp.defines: project.generalDefines.concat([name.toUpperCase() + "_LIBRARY"])
    cpp.installNamePrefix: "@rpath/PlugIns/" + provider + "/"
    cpp.rpaths: qbs.targetOS.contains("osx") ? ["@loader_path/../..", "@executable_path/.."]
                                      : ["$ORIGIN", "$ORIGIN/..", "$ORIGIN/../.."]
    cpp.cxxFlags: QtcFunctions.commonCxxFlags(qbs)
    cpp.linkerFlags: {
        var flags = QtcFunctions.commonLinkerFlags(qbs);
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            flags.push("-Wl,-s");
        else if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
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

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_plugin_path + "/" + provider
    }

    Export {
        Depends { name: "ExtensionSystem" }
        Depends { name: "cpp" }
        cpp.includePaths: [pluginIncludeBase]
    }
}
