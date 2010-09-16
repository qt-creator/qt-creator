# This file should not be edited.
# Following versions of Qt Creator might offer new version.

QT += declarative

SOURCES += $$PWD/qmlapplicationviewer.cpp
HEADERS += $$PWD/qmlapplicationviewer.h
INCLUDEPATH += $$PWD

contains(DEFINES, QMLOBSERVER) {
    CONFIG(debug, debug|release) {
        isEmpty(QMLOBSERVER_PATH) {
            warning(QMLOBSERVER_PATH was not set. QMLOBSERVER not activated.)
            DEFINES -= QMLOBSERVER
        } else {
            include($$QMLOBSERVER_PATH/qmljsdebugger-lib.pri)
        }
    } else {
        DEFINES -= QMLOBSERVER
    }
}
