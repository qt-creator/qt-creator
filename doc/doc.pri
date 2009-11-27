QDOC_BIN = $$targetPath($$[QT_INSTALL_BINS]/qdoc3)
HELPGENERATOR = $$targetPath($$[QT_INSTALL_BINS]/qhelpgenerator)

equals(QMAKE_DIR_SEP, /) {   # unix, mingw+msys
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$QDOC_BIN
} else:win32-g++* {   # just mingw
    # The lack of spaces in front of the && is necessary!
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& $$QDOC_BIN
} else {   # nmake
    QDOC = set SRCDIR=$$PWD $$escape_expand(\n\t) \
           set OUTDIR=$$OUT_PWD/doc/html $$escape_expand(\n\t) \
           $$QDOC_BIN
}

QHP_FILE = $$OUT_PWD/doc/html/qtcreator.qhp
QCH_FILE = $$IDE_DOC_PATH/qtcreator.qch

html_docs.commands = $$QDOC $$PWD/qtcreator.qdocconf
html_docs.depends += $$PWD/qtcreator.qdoc $$PWD/qtcreator.qdocconf
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o \"$$QCH_FILE\" $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

unix:!macx {
    qch_docs.path = /share/doc/qtcreator
    qch_docs.CONFIG += no_check_exist
    INSTALLS += qch_docs
}

docs.depends = qch_docs
QMAKE_EXTRA_TARGETS += html_docs qch_docs docs

OTHER_FILES = $$PWD/qtcreator.qdoc \
              $$PWD/qtcreator.qdocconf
OTHER_FILES += $$PWD/api/qtcreator-api.qdoc \
               $$PWD/api/qtcreator-api.qdocconf
