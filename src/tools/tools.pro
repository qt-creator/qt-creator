include(../../qtcreator.pri)

TEMPLATE = subdirs

SUBDIRS = qtpromaker \
     ../plugins/cpaster/frontend \
     valgrindfake \
     3rdparty \
     buildoutputparser \
     qtc-askpass

isEmpty(QTC_SKIP_SDKTOOL): SUBDIRS += sdktool

QTC_DO_NOT_BUILD_QMLDESIGNER = $$(QTC_DO_NOT_BUILD_QMLDESIGNER)
isEmpty(QTC_DO_NOT_BUILD_QMLDESIGNER):qtHaveModule(quick-private): SUBDIRS += qml2puppet

win32 {
    SUBDIRS += qtcdebugger \
        winrtdebughelper

    isEmpty(QTC_SKIP_WININTERRUPT): SUBDIRS += wininterrupt
}

mac {
    SUBDIRS += iostool
}

SUBDIRS += clangbackend

QTC_ENABLE_CLANG_REFACTORING=$$(QTC_ENABLE_CLANG_REFACTORING)
!isEmpty(QTC_ENABLE_CLANG_REFACTORING) {
    SUBDIRS += clangrefactoringbackend
    SUBDIRS += clangpchmanagerbackend
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

exists(perfparser/perfparser.pro) {
    win32 {
        SUBDIRS += perfparser
        PERFPARSER_APP_DESTDIR = $$IDE_BUILD_TREE/bin
        PERFPARSER_APP_INSTALLDIR = $$QTC_PREFIX/bin

        # On windows we take advantage of the fixed path in eblopenbackend.c: "..\lib\elfutils\"
        # So, in order to deploy elfutils with perfparser, set the following:
        # PERFPARSER_ELFUTILS_INSTALLDIR = $$QTC_PREFIX/bin
        # PERFPARSER_ELFUTILS_BACKENDS_INSTALLDIR = $$QTC_PREFIX/lib/elfutils
    } else {
        SUBDIRS += perfparser
        PERFPARSER_APP_DESTDIR = $$IDE_BUILD_TREE/libexec/qtcreator
        PERFPARSER_APP_INSTALLDIR = $$QTC_PREFIX/libexec/qtcreator

        # On linux we have "$ORIGIN/../$LIB/elfutils" in eblopenbackend.c. Unfortunately $LIB
        # can be many different things, so we target the second try where it just loads the
        # plain file name. This also allows us to put libdw and libelf in a subdir of lib.
        # So, in order to deploy elfutils with perfparser, set the following:
        # PERFPARSER_ELFUTILS_INSTALLDIR = $$QTC_PREFIX/lib/elfutils
        # PERFPARSER_ELFUTILS_BACKENDS_INSTALLDIR = $$QTC_PREFIX/lib/elfutils
    }

    cache(PERFPARSER_APP_DESTDIR)
    cache(PERFPARSER_APP_INSTALLDIR)
}

OTHER_FILES += \
    tools.qbs \
    icons/exportapplicationicons.sh \
    icons/exportdocumenttypeicons.sh

# delegate deployqt target
deployqt.CONFIG += recursive
deployqt.recurse = perfparser
QMAKE_EXTRA_TARGETS += deployqt
