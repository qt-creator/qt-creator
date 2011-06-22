TEMPLATE = subdirs

include(qmlpuppet_utilities.pri)

exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    minQtVersion(4, 7, 1) {
        SUBDIRS += qmlpuppet
    } else {
        warning(Qt 4.7.1 required!)
    }
} else {
    warning(No private headers in Qt!)
}
