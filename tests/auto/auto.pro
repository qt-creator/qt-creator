TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    extensionsystem \
    fakevim \
    generichighlighter \
#    icheckbuild \
#    profilewriter \
    ioutils \
    utils_stringutils \
    filesearch

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
