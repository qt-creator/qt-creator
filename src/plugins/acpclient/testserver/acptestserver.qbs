import qbs.FileInfo

QtApplication {
    name: "acptestserver"
    condition: qtc.withPluginTests
    Depends { name: "qtc" }
    Depends { name: "AcpLib" }
    Depends { name: "Utils" }

    consoleApplication: true
    cpp.cxxLanguageVersion: "c++20"
    cpp.rpaths: project.buildDirectory + '/' + qtc.ide_library_path

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "acptestserver.cpp",
        "acptestserver.h",
        "main.cpp",
    ]
}
