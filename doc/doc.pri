unix:QDOC_BIN = $$(QTDIR)/bin/qdoc3
win32:QDOC_BIN = $$(QTDIR)/bin/qdoc3.exe
win32:QDOC_BIN = $$replace(QDOC_BIN, "/", "\\")

# legacy branch, can be dropped as soon as we depend on Qt 4.6
!exists( $$QDOC_BIN ) {
    unix:QDOC_BIN = $$(QTDIR)/tools/qdoc3/qdoc3
    win32 { 
        QDOC_BIN = $$(QTDIR)/tools/qdoc3/release/qdoc3.exe
        QDOC_BIN = $$replace(QDOC_BIN, "/", "\\")
        !exists( $$QDOC_BIN ) {
            QDOC_BIN = $$(QTDIR)/tools/qdoc3/debug/qdoc3.exe
            QDOC_BIN = $$replace(QDOC_BIN, "/", "\\")
        }
    }
}

unix {
    QDOC = SRCDIR=$$PWD OUTDIR=$$OUT_PWD/doc/html $$QDOC_BIN 
    HELPGENERATOR = $$(QTDIR)/bin/qhelpgenerator
} else {
    QDOC = set SRCDIR=$$PWD&& set OUTDIR=$$OUT_PWD/doc/html&& $$QDOC_BIN
    # Always run qhelpgenerator inside its own cmd; this is a workaround for
    # an unusual bug which causes qhelpgenerator.exe to do nothing
    HELPGENERATOR = cmd /C $$(QTDIR)\bin\qhelpgenerator.exe
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

    qch_docs.path = /share/doc/qtcreator
    INSTALLS += qch_docs
}

macx {
    DOC_DIR = "$${OUT_PWD}/bin/Qt Creator.app/Contents/Resources/doc"
    cp_docs.commands = mkdir -p \"$${DOC_DIR}\" ; $${QMAKE_COPY} \"$${QCH_FILE}\" \"$${DOC_DIR}\"
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
OTHER_FILES += api/qtcreator-api.qdoc \
               api/qtcreator-api.qdocconf
