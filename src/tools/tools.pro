TEMPLATE = subdirs

win32:SUBDIRS = qtcdebugger
SUBDIRS += qtpromaker \
    qmlprofilertool
SUBDIRS += qmlpuppet

!win32 {
    SUBDIRS += valgrindfake
}

linux-* {
    SUBDIRS += mdnssd
}

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    SUBDIRS += qtcrashhandler
}

