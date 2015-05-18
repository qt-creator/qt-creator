TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     ../plugins/cpaster/frontend \
     sdktool \
     valgrindfake \
     3rdparty \
     qml2puppet \
     buildoutputparser

win32 {
    SUBDIRS += qtcdebugger \
        wininterrupt \
        winrtdebughelper
}

mac {
    SUBDIRS += iostool
}

BUILD_CPLUSPLUS_TOOLS = $$(BUILD_CPLUSPLUS_TOOLS)
!isEmpty(BUILD_CPLUSPLUS_TOOLS) {
    SUBDIRS += cplusplus-ast2png \
        cplusplus-frontend \
        cplusplus-mkvisitor \
        cplusplus-update-frontend
}

QT_BREAKPAD_ROOT_PATH = $$(QT_BREAKPAD_ROOT_PATH)
!isEmpty(QT_BREAKPAD_ROOT_PATH) {
    SUBDIRS += qtcrashhandler
} else {
    linux-* {
        # Build only in debug mode.
        debug_and_release|CONFIG(debug, debug|release) {
            SUBDIRS += qtcreatorcrashhandler
        }
    }
}
