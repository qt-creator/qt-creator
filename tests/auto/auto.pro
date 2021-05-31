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
    pointeralgorithm \
    profilewriter \
    ssh \
    treeviewfind \
    toolchaincache \
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
