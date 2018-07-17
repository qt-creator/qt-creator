# Creates targets for building Qt Creator documentation
#
# Usage: Include qtcreator.pri and define DOC_FILES to point to a list of qdocconf files,
#        then include this .pri file

isEmpty(VERSION): error("Include qtcreator.pri before including docs.pri!")

qtcname.name = IDE_DISPLAY_NAME
qtcname.value = "$$IDE_DISPLAY_NAME"
qtcid.name = IDE_ID
qtcid.value = $$IDE_ID
qtccasedid.name = IDE_CASED_ID
qtccasedid.value = $$IDE_CASED_ID
qtcver.name = QTC_VERSION
qtcver.value = $$QTCREATOR_DISPLAY_VERSION
qtcvertag.name = QTC_VERSION_TAG
qtcvertag.value = $$replace(VERSION, \.,)
qtdocs.name = QT_INSTALL_DOCS
qtdocs.value = $$[QT_INSTALL_DOCS/src]
qdocindex.name = QDOC_INDEX_DIR
qdocindex.value = $$[QT_INSTALL_DOCS]
qtcdocsdir.name = QTC_DOCS_DIR
qtcdocsdir.value = $$IDE_SOURCE_TREE/doc
qtccopyrightyear.name = QTCREATOR_COPYRIGHT_YEAR
qtccopyrightyear.value = $$QTCREATOR_COPYRIGHT_YEAR
qtcsourcedir.name = IDE_SOURCE_TREE
qtcsourcedir.value = $$IDE_SOURCE_TREE
QDOC_ENV += qtcname \
            qtcid \
            qtccasedid \
            qtcver \
            qtcvertag \
            qtdocs \
            qdocindex \
            qtcdocsdir \
            qtccopyrightyear \
            qtcsourcedir

DOC_INDEX_PATHS += $$IDE_BUILD_TREE/doc
DOC_HTML_INSTALLDIR = $$INSTALL_DOC_PATH
DOC_QCH_OUTDIR = $$IDE_DOC_PATH
DOC_QCH_INSTALLDIR = $$INSTALL_DOC_PATH

include(doc/doc_targets.pri)
