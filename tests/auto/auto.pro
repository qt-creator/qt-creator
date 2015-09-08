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
    filesearch \
    sdktool \
    valgrind

qtHaveModule(qml): SUBDIRS += qml
qtHaveModule(quick): SUBDIRS += timeline
