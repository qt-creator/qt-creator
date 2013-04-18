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
    filesearch

!win32 {
    SUBDIRS += valgrind
}

#contains (QT_CONFIG, declarative) {
#SUBDIRS += qml
#}
