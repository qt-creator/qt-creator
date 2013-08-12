TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += plugins/qmlprofilerextension

QMAKE_EXTRA_TARGETS = docs install_docs # dummy targets for consistency
