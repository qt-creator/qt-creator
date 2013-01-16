import qbs.base 1.0

Product {
    type: "qm"
    name: "translations"
    Depends { name: "Qt.core" }
    files: "*.ts"

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "share/qtcreator/translations"
    }
}
