TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += plugins/clangstaticanalyzer

QMAKE_EXTRA_TARGETS = docs install_docs # dummy targets for consistency
