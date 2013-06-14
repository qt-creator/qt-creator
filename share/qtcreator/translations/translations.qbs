import qbs.base 1.0

Product {
    name: "Translations"
    type: "qm"
    Depends { name: "Qt.core" }
    files: "*.ts"

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_data_path + "/translations"
    }
}
