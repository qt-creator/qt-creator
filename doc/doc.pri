unix {
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$(QTDIR)/tools/qdoc3/qdoc3
    HELPGENERATOR = qhelpgenerator
} else {
    QDOC = $$(QTDIR)\tools\qdoc3\release\qdoc3.exe
    HELPGENERATOR = qhelpgenerator
}

QHP_FILE = $$OUT_PWD/doc/html/qtcreator.qhp
QCH_FILE = $$OUT_PWD/doc/qtcreator.qch

html_docs.commands =$$QDOC $$PWD/qtcreator.qdocconf
html_docs.depends += $$PWD/qtcreator.qdoc $$PWD/qtcreator.qdocconf
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o $$QCH_FILE $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

macx {
    cp_docs.commands = $${QMAKE_COPY_DIR} $${OUT_PWD}/doc $${OUT_PWD}/bin/QtCreator.app/Contents/Resources
    cp_docs.depends += qch_docs
    docs.depends = cp_docs
    QMAKE_EXTRA_TARGETS += html_docs qch_docs cp_docs docs
}
!macx {
    docs.depends = qch_docs
    QMAKE_EXTRA_TARGETS += html_docs qch_docs docs
}

