import qbs.base 1.0

Product {
    name: "QtComponents"

    Group {
        qbs.install: true
        qbs.installDir: "lib/qtcreator/qtcomponents/"
        files: [
            "*.qml",
            "qmldir",
            "custom",
            "images"
        ]
    }
}
