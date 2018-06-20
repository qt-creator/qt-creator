TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     ../plugins/cpaster/frontend \
     valgrindfake \
     3rdparty \
     buildoutputparser

isEmpty(QTC_SKIP_SDKTOOL): SUBDIRS += sdktool

qtHaveModule(quick-private): SUBDIRS += qml2puppet

win32 {
    SUBDIRS += qtcdebugger \
        winrtdebughelper

    isEmpty(QTC_SKIP_WININTERRUPT): SUBDIRS += wininterrupt
}

mac {
    SUBDIRS += iostool
}

SUBDIRS += clangbackend

QTC_ENABLE_CLANG_LIBTOOLING=$$(QTC_ENABLE_CLANG_LIBTOOLING)
!isEmpty(QTC_ENABLE_CLANG_LIBTOOLING) {
    SUBDIRS += clangrefactoringbackend
    SUBDIRS += clangpchmanagerbackend
} else {
    warning("Not building the clang refactoring backend and the pch manager backend.")
}

isEmpty(BUILD_CPLUSPLUS_TOOLS):BUILD_CPLUSPLUS_TOOLS=$$(BUILD_CPLUSPLUS_TOOLS)
!isEmpty(BUILD_CPLUSPLUS_TOOLS) {
    SUBDIRS += cplusplus-ast2png \
        cplusplus-frontend \
        cplusplus-mkvisitor \
        cplusplus-update-frontend
}

!isEmpty(BREAKPAD_SOURCE_DIR) {
    SUBDIRS += qtcrashhandler
} else {
    linux-* {
        # Build only in debug mode.
        debug_and_release|CONFIG(debug, debug|release) {
            SUBDIRS += qtcreatorcrashhandler
        }
    }
}
