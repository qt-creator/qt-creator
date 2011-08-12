TEMPLATE = subdirs

win32:SUBDIRS = qtcdebugger
SUBDIRS += qtpromaker
SUBDIRS += qmlpuppet

!win32 {
    SUBDIRS += valgrindfake
}

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    SUBDIRS += qtcrashhandler
}

