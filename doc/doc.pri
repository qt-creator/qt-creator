unix {
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$(QTDIR)/tools/qdoc3/qdoc3
    HELPGENERATOR = $$(QTDIR)/bin/qhelpgenerator
} else {
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& $$(QTDIR)\tools\qdoc3\release\qdoc3.exe
    HELPGENERATOR = $$(QTDIR)\bin\qhelpgenerator.exe
}

QHP_FILE = $$OUT_PWD/doc/html/qtcreator.qhp
QCH_FILE = $$OUT_PWD/share/doc/qtcreator/qtcreator.qch

unix {
html_docs.commands = $$QDOC $$PWD/qtcreator.qdocconf
} else {
html_docs.commands = \"$$QDOC $$PWD/qtcreator.qdocconf\"
}
html_docs.depends += $$PWD/qtcreator.qdoc $$PWD/qtcreator.qdocconf
html_docs.files = $$QHP_FILE

qch_docs.commands = $$HELPGENERATOR -o $$QCH_FILE $$QHP_FILE
qch_docs.depends += html_docs
qch_docs.files = $$QCH_FILE

unix:!macx {
    system("mkdir -p `dirname $$QCH_FILE` && touch $$QCH_FILE")
}

unix:!macx {
    qch_docs.path = /share/doc/qtcreator
    INSTALLS += qch_docs
}

macx {
    DOC_DIR = $${OUT_PWD}/bin/QtCreator.app/Contents/Resources/doc 
    cp_docs.commands = mkdir -p $${DOC_DIR} ; $${QMAKE_COPY} $${QCH_FILE} $${DOC_DIR}
    cp_docs.depends += qch_docs
    docs.depends = cp_docs
    QMAKE_EXTRA_TARGETS += html_docs qch_docs cp_docs docs
}
!macx {
    docs.depends = qch_docs
    QMAKE_EXTRA_TARGETS += html_docs qch_docs docs
}

OTHER_FILES = qtcreator.qdoc \
              qtcreator.qdocconf
