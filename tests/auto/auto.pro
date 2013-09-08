TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    diff \
    extensionsystem \
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
