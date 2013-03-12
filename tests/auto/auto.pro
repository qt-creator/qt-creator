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
    ioutils \
    qtcprocess \
    utils \
    utils_stringutils \
    filesearch

#contains (QT_CONFIG, declarative) {
#SUBDIRS += qml
#}
