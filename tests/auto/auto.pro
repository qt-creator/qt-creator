TEMPLATE = subdirs

SUBDIRS += \
    algorithm \
    aggregation \
    changeset \
    clangtools \
    cplusplus \
    debugger \
    diff \
    extensionsystem \
    externaltool \
    environment \
    generichighlighter \
    pointeralgorithm \
    profilewriter \
    treeviewfind \
    toolchaincache \
    qtcprocess \
    json \
    utils \
    filesearch \
    mapreduce \
    runextensions \
    sdktool \
    valgrind

qtHaveModule(qml): SUBDIRS += qml
qtHaveModule(quick): SUBDIRS += flamegraph timeline
