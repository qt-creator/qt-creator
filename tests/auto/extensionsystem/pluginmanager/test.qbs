import qbs

QtcAutotest {
    name: "PluginManager autotest"
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }

    files: "tst_pluginmanager.cpp"
    cpp.defines: base.concat(['PLUGINMANAGER_TESTS_DIR="' + destinationDirectory + '"'])
}
