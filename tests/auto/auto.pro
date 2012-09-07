TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    extensionsystem \
    environment \
    fakevim \
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
