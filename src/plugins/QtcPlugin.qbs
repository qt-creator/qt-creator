import qbs.base 1.0
import qbs.FileInfo
import "../../qbs/defaults.js" as Defaults

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    property string provider: 'QtProject'
    property var pluginspecreplacements
    property var pluginRecommends: []

    targetName: Defaults.qtLibraryName(qbs, name)

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginspec" }
    Depends { name: "cpp" }
    Depends {
        condition: Defaults.testsEnabled(qbs)
        name: "Qt.test"
    }

    cpp.defines: Defaults.defines(qbs).concat([name.toUpperCase() + "_LIBRARY"])
    cpp.installNamePrefix: "@rpath/PlugIns/" + provider + "/"
    cpp.rpaths: qbs.targetOS.contains("osx") ? ["@loader_path/../..", "@executable_path/.."]
                                      : ["$ORIGIN", "$ORIGIN/..", "$ORIGIN/../.."]
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            return ["-Wl,-s"]
    }
    cpp.includePaths: [ ".", ".." ]

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
    }
}
