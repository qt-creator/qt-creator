TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += plugins/autotest shared

QMAKE_EXTRA_TARGETS = docs install_docs # dummy targets for consistency
