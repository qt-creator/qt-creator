import qbs.base 1.0
import qbs.fileinfo as FileInfo
import "../../qbs/defaults.js" as Defaults

Product {
    type: ["dynamiclibrary", "pluginSpec"]
    property string provider: 'QtProject'
    property var pluginspecreplacements
    property var pluginRecommends: []
    targetName: {
        // see PluginSpecPrivate::loadLibrary()
        if (qbs.debugInformation) {
            switch (qbs.targetOS) {
            case "windows":
                return name + "d";
            case "mac":
                return name + "_debug";
            }
        }
        return name;
    }

    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginspec" }
    Depends { name: "cpp" }
    Depends {
        condition: Defaults.testsEnabled(qbs)
        name: "Qt.test"
    }

    cpp.defines: Defaults.defines(qbs).concat([name.toUpperCase() + "_LIBRARY"])
    cpp.rpaths: ["$ORIGIN/../.."]
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }
    cpp.includePaths: [ ".", ".." ]

    Group {
        name: "PluginSpec"
        files: [ product.name + ".pluginspec.in" ]
        fileTags: ["pluginSpecIn"]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_plugin_path + "/" + provider
    }

    ProductModule {
        Depends { name: "ExtensionSystem" }
    }
}
