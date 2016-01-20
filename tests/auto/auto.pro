TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    clangstaticanalyzer \
    cplusplus \
    debugger \
    diff \
    extensionsystem \
    externaltool \
    environment \
    generichighlighter \
    profilewriter \
    treeviewfind \
    qtcprocess \
    json \
    utils \
    filesearch \
    runextensions \
    sdktool \
    valgrind

qtHaveModule(qml): SUBDIRS += qml
qtHaveModule(quick): SUBDIRS += timeline
