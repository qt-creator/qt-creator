QDOC_BIN = $$[QT_INSTALL_BINS]/qdoc3
win32:QDOC_BIN = $$replace(QDOC_BIN, "/", "\\")

unix {
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$QDOC_BIN 
    HELPGENERATOR = $$[QT_INSTALL_BINS]/qhelpgenerator
} else {
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& $$QDOC_BIN
    # Always run qhelpgenerator inside its own cmd; this is a workaround for
    # an unusual bug which causes qhelpgenerator.exe to do nothing
    HELPGENERATOR = cmd /C $$replace($$list($$[QT_INSTALL_BINS]/qhelpgenerator.exe), "/", "\\")
}

QHP_FILE = $$OUT_PWD/doc/html/qtcreator.qhp
QCH_FILE = $$IDE_DOC_PATH/qtcreator.qch

unix {
html_docs.commands = $$QDOC $$PWD/qtcreator.qdocconf
} else {
html_docs.commands = \"$$QDOC $$PWD/qtcreator.qdocconf\"
}
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
