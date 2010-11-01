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
    utils_stringutils \
    filesearch

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
