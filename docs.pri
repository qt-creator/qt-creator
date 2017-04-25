# Creates targets for building Qt Creator documentation
#
# Usage: Include qtcreator.pri and define DOC_FILES to point to a list of qdocconf files,
#        then include this .pri file

isEmpty(VERSION): error("Include qtcreator.pri before including docs.pri!")

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
QDOC_ENV += qtcver qtcvertag qtdocs qdocindex qtcdocsdir

DOC_INDEX_PATHS += $$IDE_BUILD_TREE/doc
DOC_HTML_INSTALLDIR = $$INSTALL_DOC_PATH
DOC_QCH_OUTDIR = $$IDE_DOC_PATH
DOC_QCH_INSTALLDIR = $$INSTALL_DOC_PATH

include(doc/doc_targets.pri)
