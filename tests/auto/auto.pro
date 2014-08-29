TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    diff \
    extensionsystem \
    externaltool \
    environment \
    generichighlighter \
    profilewriter \
    treeviewfind \
    ioutils \
    qtcprocess \
    utils \
    utils_stringutils \
    filesearch \
    valgrind

qtHaveModule(declarative) {
    SUBDIRS += qml qmlprofiler
}
