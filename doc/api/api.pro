TEMPLATE = app
TARGET = apidoc
CONFIG -= qt
QT =
LIBS =
macx:CONFIG -= app_bundle

isEmpty(vcproj) {
    QMAKE_LINK = @: IGNORE THIS LINE
    OBJECTS_DIR =
    win32:CONFIG -= embed_manifest_exe
} else {
    CONFIG += console
    PHONY_DEPS = .
    phony_src.input = PHONY_DEPS
    phony_src.output = phony.c
    phony_src.variable_out = GENERATED_SOURCES
    phony_src.commands = echo int main() { return 0; } > phony.c
    phony_src.name = CREATE phony.c
    phony_src.CONFIG += combine
    QMAKE_EXTRA_COMPILERS += phony_src
}

include(../../qtcreator.pri)
QDOC_BIN = $$targetPath($$[QT_INSTALL_BINS]/qdoc3)
HELPGENERATOR = $$targetPath($$[QT_INSTALL_BINS]/qhelpgenerator)

VERSION_TAG = $$replace(QTCREATOR_VERSION, "[-.]", )

equals(QMAKE_DIR_SEP, /) {   # unix, mingw+msys
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html QTC_VERSION=$$QTCREATOR_VERSION QTC_VERSION_TAG=$$VERSION_TAG $$QDOC_BIN
} else:win32-g++* {   # just mingw
    # The lack of spaces in front of the && is necessary!
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& set QTC_VERSION=$$QTCREATOR_VERSION&& set QTC_VERSION_TAG=$$VERSION_TAG&& $$QDOC_BIN
} else {   # nmake
    QDOC = set SRCDIR=$$PWD $$escape_expand(\\n\\t) \
           set OUTDIR=$$OUT_PWD/doc/html $$escape_expand(\\n\\t) \
           set QTC_VERSION=$$QTCREATOR_VERSION $$escape_expand(\\n\\t) \
           set QTC_VERSION_TAG=$$VERSION_TAG $$escape_expand(\\n\\t) \
           $$QDOC_BIN
}

HELP_FILES     = $$PWD/qtcreator-dev.qdocconf
HELP_DEP_FILES = $$PWD/qtcreator-api.qdoc \
                 $$PWD/coding-style.qdoc \
                 $$PWD/external-tool-spec.qdoc \
                 $$PWD/qtcreator-dev.qdoc \
                 $$PWD/qtcreator-dev-wizards.qdoc \
                 $$PWD/qtcreator-dev.qdocconf

docs.name = CREATE API DOC
docs.input = HELP_FILES
docs.output = $$OUT_PWD/index.html
docs.depends = $$HELP_DEP_FILES
win32:docs.commands = $$QDOC \"${QMAKE_FILE_IN}\"
unix:docs.commands = $$QDOC ${QMAKE_FILE_IN}
docs.CONFIG += no_link
isEmpty(vcproj):docs.variable_out = PRE_TARGETDEPS
QMAKE_EXTRA_COMPILERS += docs
