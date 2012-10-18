TEMPLATE = subdirs

include(../../../qtcreator.pri)
include(../../private_headers.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += declarative-private core-private
    SUBDIRS += qmlpuppet
} else {
    exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
        minQtVersion(4, 7, 1) {
            SUBDIRS += qmlpuppet
        }
    }
}
