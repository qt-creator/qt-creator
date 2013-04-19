import qbs.base 1.0

Product {
    name: "QtComponents"

    Group {
        qbs.install: true
        qbs.installDir: (qbs.targetOS == "windows" ? "lib/qtcreator" : project.ide_library_path)
                        + "/qtcomponents"
        files: [
            "*.qml",
            "qmldir",
            "custom",
            "images"
        ]
    }
}
