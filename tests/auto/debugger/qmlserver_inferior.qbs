import qbs.FileInfo

QtApplication {
    name: "qmlserver_inferior"
    condition: Qt.quick.present
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.qml" }

    cpp.defines: ["QT_QML_DEBUG"]

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qmlserver_inferior.cpp",
        "qmlserver_inferior.qrc",
    ]
}
