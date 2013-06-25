import qbs.base 1.0

Product {
    name: "QtComponents"

    Group {
        name: "Resources"
        qbs.install: true
        qbs.installDir: (qbs.targetOS.contains("windows") ? "lib/qtcreator" : project.ide_library_path)
                        + "/qtcomponents"
        files: [
            "*.qml",
            "qmldir",
            "custom",
            "images"
        ]
    }
}
