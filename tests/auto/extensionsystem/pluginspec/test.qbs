import qbs
import QtcAutotest

QtcAutotest {
    name: "ExtensionSystem pluginspec autotest"
    Depends { name: "Aggregation" }
    Depends { name: "ExtensionSystem" }
    Depends { name: "pluginspec_test" }
    cpp.defines: base.concat(['PLUGINSPEC_DIR="' + destinationDirectory + '"'])
    files: "tst_pluginspec.cpp"
    Group {
        id: testSpecsGroup
        name: "test specs"
        fileTags: "copyable_resource"
        copyable_resource.targetDirectory: product.destinationDirectory + "/testspecs"
        files: [
            "testspecs/simplespec.xml",
            "testspecs/simplespec_experimental.xml",
            "testspecs/spec1.xml",
            "testspecs/spec2.xml",
            "testspecs/spec_wrong1.xml",
            "testspecs/spec_wrong2.xml",
            "testspecs/spec_wrong3.xml",
            "testspecs/spec_wrong4.xml",
            "testspecs/spec_wrong5.xml",
        ]
    }
    Group {
        id: testDependenciesGroup
        name: "test dependencies"
        fileTags: "copyable_resource"
        copyable_resource.targetDirectory: product.destinationDirectory + "/testdependencies"
        files: [
            "testdependencies/spec1.xml",
            "testdependencies/spec2.xml",
            "testdependencies/spec3.xml",
            "testdependencies/spec4.xml",
            "testdependencies/spec5.xml",
        ]
    }
    Group {
        id: specGroup
        name: "spec"
        fileTags: "copyable_resource"
        copyable_resource.targetDirectory: product.destinationDirectory + "/testdir"
        files: ["testdir/spec.xml"]
    }
}
