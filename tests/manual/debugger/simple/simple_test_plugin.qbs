import qbs.FileInfo

DynamicLibrary {
    name: "Manual Test Simple Plugin"
    targetName: "simple_test_plugin"

    Depends { name: "Qt.core" }

    files: [ "simple_test_plugin.cpp" ]

    destinationDirectory: FileInfo.joinPaths(
                              FileInfo.path(project.buildDirectory + '/'
                                            + FileInfo.relativePath(project.ide_source_tree,
                                                                    sourceDirectory)),
                              "simple")
}
