TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     ../plugins/cpaster/frontend \
     sdktool \
     valgrindfake \
     3rdparty \
     buildoutputparser

qtHaveModule(quick): SUBDIRS += qml2puppet

win32 {
    SUBDIRS += qtcdebugger \
        winrtdebughelper

    isEmpty(QTC_SKIP_WININTERRUPT): SUBDIRS += wininterrupt
}

mac {
    SUBDIRS += iostool
}

isEmpty(LLVM_INSTALL_DIR):LLVM_INSTALL_DIR=$$(LLVM_INSTALL_DIR)
exists($$LLVM_INSTALL_DIR) {
    SUBDIRS += clangbackend

    QTC_NO_CLANG_LIBTOOLING=$$(QTC_NO_CLANG_LIBTOOLING)
    isEmpty(QTC_NO_CLANG_LIBTOOLING) {
        SUBDIRS += clangrefactoringbackend
        SUBDIRS += clangpchmanagerbackend
    } else {
        warning("Building the Clang refactoring back end and the pch manager plugins are disabled.")
    }
}

isEmpty(BUILD_CPLUSPLUS_TOOLS):BUILD_CPLUSPLUS_TOOLS=$$(BUILD_CPLUSPLUS_TOOLS)
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
