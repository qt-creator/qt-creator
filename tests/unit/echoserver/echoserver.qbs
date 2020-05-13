import qbs.FileInfo

QtcProduct {
    name: "echoserver"
    type: "application"
    targetName: "echo"
    consoleApplication: true
    destinationDirectory: FileInfo.joinPaths(project.buildDirectory,
        FileInfo.relativePath(project.ide_source_tree, sourceDirectory))
    install: false

    Depends { name: "qtc" }
    Depends { name: "ClangSupport" }
    Depends { name: "Sqlite" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }

    cpp.defines: ["CLANGSUPPORT_TESTS", "DONT_CHECK_MESSAGE_COUNTER"]
    cpp.dynamicLibraries: qbs.targetOS.contains("unix:") ? ["dl"] : []
    cpp.rpaths: [
        FileInfo.joinPaths(project.buildDirectory, qtc.ide_library_path),
        FileInfo.joinPaths(project.buildDirectory, qtc.ide_plugin_path)
    ]

    files: [
        "echoclangcodemodelserver.cpp",
        "echoclangcodemodelserver.h",
        "echoserverprocessmain.cpp",
    ]
}
