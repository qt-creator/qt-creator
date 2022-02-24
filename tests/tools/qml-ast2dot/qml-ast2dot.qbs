import qbs.FileInfo

CppApplication {
    Depends { name: "QmlJS" }
    Depends { name: "Qt.gui" }

    cpp.cxxLanguageVersion: "c++17"
    consoleApplication: true
    targetName: "qml-ast2dot"
    builtByDefault: false

    files: [
        "main.cpp"
    ]

   destinationDirectory: FileInfo.joinPaths(
                             FileInfo.path(project.buildDirectory + '/'
                                           + FileInfo.relativePath(project.ide_source_tree,
                                                                   sourceDirectory)),
                             "qml-ast2dot")
}
