TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
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
