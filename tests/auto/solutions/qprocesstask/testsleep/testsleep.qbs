import qbs.FileInfo

QtApplication {
    name: "testsleep"

    consoleApplication: true

    install: false
    files: [
        "main.cpp",
    ]

    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)
}
