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
