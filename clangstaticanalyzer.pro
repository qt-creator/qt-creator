TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += \
    plugins/clangstaticanalyzer \
    plugins/clangstaticanalyzer/tests

QMAKE_EXTRA_TARGETS = docs install_docs # dummy targets for consistency
