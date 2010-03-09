# Try to find location of Qt private headers (see README)
isEmpty(QT_PRIVATE_HEADERS) {
    QT_PRIVATE_HEADERS = $$[QT_INSTALL_HEADERS]
} else {
    INCLUDEPATH += $$quote($${QT_PRIVATE_HEADERS}) \
                   $$quote($${QT_PRIVATE_HEADERS}/QtDeclarative) \
                   $$quote($${QT_PRIVATE_HEADERS}/QtCore) \
                   $$quote($${QT_PRIVATE_HEADERS}/QtScript)
}
