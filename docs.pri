# adapted from qt_docs.prf

isEmpty(VERSION): error("Include qtcreator.pri before including docs.pri!")

qtcver.name = QTC_VERSION
qtcver.value = $$VERSION
qtcvertag.name = QTC_VERSION_TAG
qtcvertag.value = $$replace(qtcver.value, \.,)
qtdocs.name = QT_INSTALL_DOCS
qtdocs.value = $$[QT_INSTALL_DOCS/src]
qdocindex.name = QDOC_INDEX_DIR
qdocindex.value = $$[QT_INSTALL_DOCS]
qtcdocsdir.name = QTC_DOCS_DIR
qtcdocsdir.value = $$IDE_SOURCE_TREE/doc
QT_TOOL_ENV = qtcver qtcvertag qtdocs qdocindex qtcdocsdir
qtPrepareTool(QDOC, qdoc)
QT_TOOL_ENV =

!build_online_docs: qtPrepareTool(QHELPGENERATOR, qhelpgenerator)

QTC_DOCS_BASE_OUTDIR = $$OUT_PWD/doc
DOC_INDEXES = -indexdir $$shell_quote($$[QT_INSTALL_DOCS]) \
              -indexdir $$shell_quote($$IDE_BUILD_TREE/doc)

for (qtc_doc, QTC_DOCS) {
    !exists($$qtc_doc): error("Cannot find documentation specification file $$qtc_doc")
    QTC_DOCS_TARGET = $$replace(qtc_doc, ^(.*/)?(.*)\\.qdocconf$, \\2)
    QTC_DOCS_TARGETDIR = $$QTC_DOCS_TARGET
    QTC_DOCS_OUTPUTDIR = $$QTC_DOCS_BASE_OUTDIR/$$QTC_DOCS_TARGETDIR

    html_docs_$${QTC_DOCS_TARGET}.commands = $$QDOC -outputdir $$shell_quote($$QTC_DOCS_OUTPUTDIR) $$qtc_doc $$DOC_INDEXES
    QMAKE_EXTRA_TARGETS += html_docs_$${QTC_DOCS_TARGET}

    !isEmpty(html_docs.commands): html_docs.commands += &&
    html_docs.commands += $$eval(html_docs_$${QTC_DOCS_TARGET}.commands)

    !build_online_docs {
        qch_docs_$${QTC_DOCS_TARGET}.commands = $$QHELPGENERATOR $$shell_quote($$QTC_DOCS_OUTPUTDIR/$${QTC_DOCS_TARGET}.qhp) -o $$shell_quote($$IDE_DOC_PATH/$${QTC_DOCS_TARGET}.qch)
        qch_docs_$${QTC_DOCS_TARGET}.depends = html_docs_$${QTC_DOCS_TARGET}
        QMAKE_EXTRA_TARGETS += qch_docs_$${QTC_DOCS_TARGET}

        !isEmpty(qch_docs.commands): qch_docs.commands += &&
        qch_docs.commands += $$eval(qch_docs_$${QTC_DOCS_TARGET}.commands)

        inst_qch_docs.files += $$IDE_DOC_PATH/$${QTC_DOCS_TARGET}.qch
    }
}

!build_online_docs {
    qch_docs.depends = html_docs
    inst_qch_docs.path = $$INSTALL_DOC_PATH
    inst_qch_docs.CONFIG += no_check_exist no_default_install no_build
    install_docs.depends = install_inst_qch_docs
    docs.depends = qch_docs
    INSTALLS += inst_qch_docs
    QMAKE_EXTRA_TARGETS += qch_docs install_docs
} else {
    docs.depends = html_docs
}

QMAKE_EXTRA_TARGETS += html_docs docs
