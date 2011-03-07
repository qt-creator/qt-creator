# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

QT += declarative script
INCLUDEPATH += $$PWD/include

DEBUGLIB=QmlJSDebugger
CONFIG(debug, debug|release) {
    windows:DEBUGLIB = QmlJSDebuggerd
}
LIBS += -L$$PWD -l$$DEBUGLIB

DEFINES += QMLJSDEBUGGER
