# This file should not be edited.
# Following versions of Qt Creator might offer new version.

QT += declarative

SOURCES += $$PWD/qmlapplicationviewer.cpp
HEADERS += $$PWD/qmlapplicationviewer.h
INCLUDEPATH += $$PWD

contains(DEFINES, QMLINSPECTOR) {
    CONFIG(debug, debug|release) {
        isEmpty(QMLINSPECTOR_PATH) {
            warning(QMLINSPECTOR_PATH was not set. QMLINSPECTOR not activated.)
            DEFINES -= QMLINSPECTOR
        } else {
            include($$QMLINSPECTOR_PATH/qmljsdebugger-lib.pri)
        }
    } else {
        DEFINES -= QMLINSPECTOR
    }
}
