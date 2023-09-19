import qbs.FileInfo

DynamicLibrary {
    condition: !qtc.present || qtc.withAutotests
    name: "Manual Test Simple Plugin"
    targetName: "simple_test_plugin"

    Depends { name: "qtc"; required: false }
    Depends { name: "Qt.core" }

    files: [ "simple_test_plugin.cpp" ]

    destinationDirectory: FileInfo.joinPaths(
                              FileInfo.path(project.buildDirectory + '/'
                                            + FileInfo.relativePath(project.ide_source_tree,
                                                                    sourceDirectory)),
                              "simple")
}
