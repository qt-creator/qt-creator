TEMPLATE = app
TARGET = pluginhowto
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


QDOC_BIN = $$[QT_INSTALL_BINS]/qdoc3
equals(QMAKE_DIR_SEP, /) {   # unix, mingw+msys
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/html $$QDOC_BIN
} else:win32-g++* {   # just mingw
    # The lack of spaces in front of the && is necessary!
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/html&& $$QDOC_BIN
} else {   # nmake
    QDOC = set SRCDIR=$$PWD $$escape_expand(\\n\\t) \
           set OUTDIR=$$OUT_PWD/html $$escape_expand(\\n\\t) \
           $$QDOC_BIN
}

HELP_FILES     = $$PWD/qtcreator-pluginhowto.qdocconf
HELP_DEP_FILES = $$PWD/adv-find-filter.qdoc \
                 $$PWD/editor-type.qdoc \
                 $$PWD/find-filter.qdoc \
                 $$PWD/find-support.qdoc \
                 $$PWD/first-plugin.qdoc \
                 $$PWD/location-filter.qdoc \
                 $$PWD/menu.qdoc \
                 $$PWD/mode.qdoc \
                 $$PWD/nav-widget.qdoc \
                 $$PWD/pref-pane.qdoc \
                 $$PWD/progress.qdoc \
                 $$PWD/project-file-wizard.qdoc \
                 $$PWD/qtc-arch.qdoc \
                 $$PWD/qtc-project-plugin.qdoc \
                 $$PWD/qtcreator-compile.qdoc \
                 $$PWD/qtcreator-pluginhowto.qdoc \
                 $$PWD/vcs.qdoc \
                 $$PWD/../config/qt-defines.qdocconf \
                 $$PWD/../config/qt-html-templates.qdocconf \
                 $$PWD/qtcreator-pluginhowto.qdocconf

docs.name = CREATE API DOC
docs.input = HELP_FILES
docs.output = $$OUT_PWD/index.html
docs.depends = $$HELP_DEP_FILES
win32:docs.commands = $$QDOC \"${QMAKE_FILE_IN}\"
unix:docs.commands = $$QDOC ${QMAKE_FILE_IN}
docs.CONFIG += no_link
isEmpty(vcproj):docs.variable_out = PRE_TARGETDEPS
QMAKE_EXTRA_COMPILERS += docs
