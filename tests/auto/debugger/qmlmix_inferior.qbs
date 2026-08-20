import qbs.FileInfo

QtApplication {
    name: "qmlmix_inferior"
    condition: Qt.quick.present

    Depends { name: "Qt.quick"; required: false }

    Qt.qml.importName: "MixTest"
    Qt.qml.importVersion: "1.0"

    cpp.defines: ["QT_QML_DEBUG"]
    cpp.includePaths: base.concat("qmlmix_inferior")

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qmlmix_inferior.h",
        "qmlmix_inferior.cpp",
    ]

    Group {
        name: "qmlmix QML sources"
        Qt.core.resourceSourceBase: path
        files: ["Main.qml"]
        fileTags: ["qt.qml.qml", "qt.core.resource_data"]
    }
}
