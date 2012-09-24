import qbs.base 1.0

Product {
    type: "qm"
    name: "translations"
    Depends { name: "Qt.core" }
    destination: "share/qtcreator/translations"
    files: "*.ts"
}
