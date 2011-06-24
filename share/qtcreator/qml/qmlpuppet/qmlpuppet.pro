TEMPLATE = subdirs

include(qmlpuppet_utilities.pri)

exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    minQtVersion(4, 7, 1) {
        SUBDIRS += qmlpuppet
    } else {
        warning(Qt version has to be at least 4.7.1 to build qmlpuppet.)
    }
} else {
    warning(Private headers for Qt required to build qmlpuppet.)
}
