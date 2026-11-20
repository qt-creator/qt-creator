import qbs.FileInfo

QtApplication {
    name: "qtstring_inferior"

    cpp.optimization: "none"
    cpp.debugInformation: true

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qtstring_inferior.cpp",
    ]
}
