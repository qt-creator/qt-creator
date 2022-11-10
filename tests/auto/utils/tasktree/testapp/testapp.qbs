import qbs.FileInfo

QtApplication {
    name: "testapp"
    Depends { name: "qtc" }
    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    consoleApplication: true
    cpp.cxxLanguageVersion: "c++17"
    cpp.rpaths: project.buildDirectory + '/' + qtc.ide_library_path

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "main.cpp",
    ]
}
