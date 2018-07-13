TEMPLATE = subdirs

SUBDIRS += \
    algorithm \
    aggregation \
    changeset \
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
    languageserverprotocol \
    sdktool \
    valgrind

qtHaveModule(qml): SUBDIRS += qml
qtHaveModule(quick): SUBDIRS += tracing
