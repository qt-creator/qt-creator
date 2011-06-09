TEMPLATE = subdirs

include(../../../qtcreator.pri)
include(../../private_headers.pri)

minQtVersion(5, 0, 0) {
    SUBDIRS += qml2puppet
}

exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    minQtVersion(4, 7, 1) {
        SUBDIRS += qmlpuppet
    }
}
