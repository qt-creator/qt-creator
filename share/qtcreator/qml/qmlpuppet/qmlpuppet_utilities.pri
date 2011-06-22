# Try to find location of Qt private headers (see README)
isEmpty(QT_PRIVATE_HEADERS) {
    QT_PRIVATE_HEADERS = $$[QT_INSTALL_HEADERS]
} else {
    INCLUDEPATH += \
        $${QT_PRIVATE_HEADERS} \
        $${QT_PRIVATE_HEADERS}/QtCore \
        $${QT_PRIVATE_HEADERS}/QtGui \
        $${QT_PRIVATE_HEADERS}/QtScript \
        $${QT_PRIVATE_HEADERS}/QtDeclarative
}

defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    return(false)
}
