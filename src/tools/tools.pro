TEMPLATE = subdirs

win32:SUBDIRS = qtcdebugger
SUBDIRS += qtpromaker

!win32 {
    SUBDIRS += valgrindfake
}

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    SUBDIRS += qtcrashhandler
}

include(../../qtcreator.pri)
include(../private_headers.pri)
exists($${QT_PRIVATE_HEADERS}/QtDeclarative/private/qdeclarativecontext_p.h) {
    minQtVersion(4, 7, 1) {
        SUBDIRS += qmlpuppet
    }
}
