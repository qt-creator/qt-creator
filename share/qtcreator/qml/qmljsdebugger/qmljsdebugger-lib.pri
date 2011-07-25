# This file is part of Qt Creator
# It enables debugging of Qt Quick applications

QT += declarative script
INCLUDEPATH += $$PWD/include

symbian {
    LIBNAME = QmLJSDebugger.lib
} else {
    windows:CONFIG(debug, debug|release) {
        LIBNAME = QmlJSDebuggerd
    } else {
        LIBNAME = QmlJSDebugger
    }
}
LIBS += -L$$PWD -l$$LIBNAME

symbian {
    # Work around bug in gcce toolchain (QTCREATORBUG-5589)
    LIBS += -lusrt2_2.lib
}

DEFINES += QMLJSDEBUGGER
