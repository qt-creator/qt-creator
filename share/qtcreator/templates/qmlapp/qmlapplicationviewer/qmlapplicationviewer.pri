# This file should not be edited.
# Following versions of Qt Creator might offer new version.

QT += declarative

SOURCES += $$PWD/qmlapplicationviewer.cpp
HEADERS += $$PWD/qmlapplicationviewer.h
INCLUDEPATH += $$PWD

contains(DEFINES, QMLOBSERVER) {
    DEFINES *= QMLJSDEBUGGER
}
contains(DEFINES, QMLJSDEBUGGER) {
    CONFIG(debug, debug|release) {
        isEmpty(QMLJSDEBUGGER_PATH) {
            warning(QMLJSDEBUGGER_PATH was not set. QMLJSDEBUGGER not activated.)
            DEFINES -= QMLJSDEBUGGER
        } else {
            include($$QMLJSDEBUGGER_PATH/qmljsdebugger-lib.pri)
        }
    } else {
        DEFINES -= QMLJSDEBUGGER
    }
}
