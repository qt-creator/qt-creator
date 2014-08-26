import qbs
import QtcAutotest

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

    files: "tst_pluginmanager.cpp"
    cpp.defines: base.concat(['PLUGINMANAGER_TESTS_DIR="' + destinationDirectory + '"'])
}
