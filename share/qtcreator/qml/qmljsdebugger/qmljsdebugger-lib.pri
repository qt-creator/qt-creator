# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

QT += declarative script
INCLUDEPATH += $$PWD/include

windows:CONFIG(debug, debug|release) {
    LIBNAME = QmlJSDebuggerd
} else {
    LIBNAME = QmlJSDebugger
}
LIBS += -L$$PWD -l$$LIBNAME

DEFINES += QMLJSDEBUGGER
