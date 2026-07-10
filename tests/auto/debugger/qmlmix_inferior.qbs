import qbs.FileInfo

QtApplication {
    name: "qmlmix_inferior"
    condition: Qt.quick.present
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.qml" }

    Qt.qml.importName: "MixTest"
    Qt.qml.importVersion: "1.0"

    cpp.defines: ["QT_QML_DEBUG"]

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qmlmix_inferior/qmlentrypoint.h",
        "qmlmix_inferior/qmlentrypoint.cpp",
        "qmlmix_inferior/main.cpp",
    ]

    Group {
        name: "qmlmix QML sources"
        Qt.core.resourceSourceBase: FileInfo.joinPaths(path, "qmlmix_inferior")
        files: ["qmlmix_inferior/Main.qml"]
        fileTags: ["qt.qml.qml", "qt.core.resource_data"]
    }
}
