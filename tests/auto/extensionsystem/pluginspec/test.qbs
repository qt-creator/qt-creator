import qbs

import QtcAutotest
import "../copytransformer.qbs" as CopyTransformer

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
        files: ["testdir/spec.xml"]
    }

    CopyTransformer {
        sourceFiles: testSpecsGroup.files
        targetDirectory: product.destinationDirectory + "/testspecs"
    }

    CopyTransformer {
        sourceFiles: testDependenciesGroup.files
        targetDirectory: product.destinationDirectory + "/testdependencies"
    }

    CopyTransformer {
        sourceFiles: specGroup.files
        targetDirectory: product.destinationDirectory + "/testdir"
    }
}
