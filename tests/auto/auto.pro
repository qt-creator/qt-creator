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

#contains (QT_CONFIG, declarative) {
#SUBDIRS += qml
#}
