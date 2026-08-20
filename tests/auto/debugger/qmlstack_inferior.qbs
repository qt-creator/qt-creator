import qbs.FileInfo

QtApplication {
    name: "qmlstack_inferior"
    condition: Qt.quick.present

    Depends { name: "Qt.quick"; required: false }

    cpp.defines: ["QT_QML_DEBUG"]

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qmlstack_inferior.cpp",
        "qmlstack_inferior.qrc",
    ]
}
