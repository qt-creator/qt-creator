# This file should not be edited.
# Following versions of Qt Creator might offer new version.

QT += declarative

SOURCES += $$PWD/qmlapplicationviewer.cpp
HEADERS += $$PWD/qmlapplicationviewer.h
INCLUDEPATH += $$PWD

contains(DEFINES, QMLOBSERVER) {
    DEFINES *= QMLJSDEBUGGER
}

defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    return(false)
}

contains(DEFINES, QMLJSDEBUGGER) {
    CONFIG(debug, debug|release) {
        !minQtVersion(4, 7, 1) {
            warning()
            warning("Debugging QML requires the qmljsdebugger library that ships with Qt Creator.")
            warning("This library requires Qt 4.7.1 or newer.")
            warning()

            error("Qt version $$QT_VERSION too old for QmlJS Debugging. Aborting.")
        }
        isEmpty(QMLJSDEBUGGER_PATH) {
            warning()
            warning("Debugging QML requires the qmljsdebugger library that ships with Qt Creator.")
            warning("Please specify its location on the qmake command line, eg")
            warning("  qmake -r QMLJSDEBUGGER_PATH=$CREATORDIR/share/qtcreator/qmljsdebugger")
            warning()

            error("QMLJSDEBUGGER defined, but no QMLJSDEBUGGER_PATH set on command line. Aborting.")
            DEFINES -= QMLJSDEBUGGER
        } else {
            include($$QMLJSDEBUGGER_PATH/qmljsdebugger-lib.pri)
        }
    } else {
        DEFINES -= QMLJSDEBUGGER
    }
}
