# Generate docs. Does not work for shadow builds and never will.
# (adding a "docs" make target).

unix {
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/html $$(QTDIR)/tools/qdoc3/qdoc3
    HELPGENERATOR = qhelpgenerator
} else {
    QDOC = $$(QTDIR)\tools\qdoc3\release\qdoc3.exe
    HELPGENERATOR = qhelpgenerator
}

QHP_FILE = $$OUT_PWD/html/qtcreator.qhp
QCH_FILE = $$OUT_PWD/qtcreator.qch

html_docs.commands =$$QDOC $$PWD/qtcreator.qdocconf
html_docs.depends += $$PWD/qtcreator.qdoc $$PWD/qtcreator.qdocconf
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o $$QCH_FILE $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

docs.depends = qch_docs

QMAKE_EXTRA_TARGETS += html_docs qch_docs docs
