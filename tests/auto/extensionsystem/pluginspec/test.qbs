import qbs

QtcAutotest {
    name: "ExtensionSystem pluginspec autotest"
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Group {
        name: "Sources"
        files: "tst_pluginspec.cpp"
        cpp.defines: outer.concat([
            'PLUGIN_DIR="' + destinationDirectory + '"',
            'PLUGINSPEC_DIR="' + sourceDirectory + '"'
        ])
    }

    Group {
        id: testSpecsGroup
        name: "test specs"
        files: [
            "testspecs/simplespec.json",
            "testspecs/simplespec_experimental.json",
            "testspecs/spec1.json",
            "testspecs/spec2.json",
            "testspecs/spec_wrong2.json",
            "testspecs/spec_wrong3.json",
            "testspecs/spec_wrong4.json",
            "testspecs/spec_wrong5.json",
        ]
    }
    Group {
        id: testDependenciesGroup
        name: "test dependencies"
        files: [
            "testdependencies/spec1.json",
            "testdependencies/spec2.json",
            "testdependencies/spec3.json",
            "testdependencies/spec4.json",
            "testdependencies/spec5.json",
        ]
    }
    Group {
        id: specGroup
        name: "spec"
        files: ["testdir/spec.json"]
    }
}
