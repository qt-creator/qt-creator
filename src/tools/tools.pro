TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     qmlpuppet \
     ../plugins/cpaster/frontend \
     sdktool \
     valgrindfake \
     3rdparty \
     buildoutputparser

win32 {
    SUBDIRS += qtcdebugger \
        wininterrupt \
        winrtdebughelper
}

mac {
    SUBDIRS += iostool
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

greaterThan(QT_MAJOR_VERSION, 4) {
    !greaterThan(QT_MINOR_VERSION, 0):!greaterThan(QT_PATCH_VERSION, 0) {
    } else {
        SUBDIRS += qml2puppet
    }
}
