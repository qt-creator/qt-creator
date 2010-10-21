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
    utils_stringutils

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
