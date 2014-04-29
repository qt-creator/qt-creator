import qbs
import QtcAutotest
import "../copytransformer.qbs" as CopyTransformer

QtcAutotest {
    name: "PluginManager autotest"
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "circular_plugin1" }
    Depends { name: "circular_plugin2" }
    Depends { name: "circular_plugin3" }
    Depends { name: "correct_plugin1" }
    Depends { name: "correct_plugin2" }
    Depends { name: "correct_plugin3" }
    Group {
        id: pluginGroup
        name: "plugins"
        files: [
            "plugins/otherplugin.xml",
            "plugins/plugin1.xml",
            "plugins/myplug/myplug.xml"
        ]
    }

    CopyTransformer {
        sourceFiles: pluginGroup.files
        targetDirectory: product.destinationDirectory + "/plugins"
    }

    files: "tst_pluginmanager.cpp"
    cpp.defines: base.concat(['PLUGINMANAGER_TESTS_DIR="' + destinationDirectory + '"'])
}
