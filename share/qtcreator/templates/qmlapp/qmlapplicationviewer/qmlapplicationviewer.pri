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
            warning()
            warning(Debugging QML requires the qmljsdebugger library that ships with Qt Creator.)
            warning(Please specify its location on the qmake command line, e.g.)
            warning(  qmake -r QMLJSDEBUGGER_PATH=$CREATORDIR/share/qtcreator/qmljsdebugger)
            warning()

            error(QMLJSDEBUGGER defined, but no QMLJSDEBUGGER_PATH set on command line. Aborting.)
            DEFINES -= QMLJSDEBUGGER
        } else {
            include($$QMLJSDEBUGGER_PATH/qmljsdebugger-lib.pri)
        }
    } else {
        DEFINES -= QMLJSDEBUGGER
    }
}
