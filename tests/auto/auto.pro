TEMPLATE = subdirs

SUBDIRS += \
    cplusplus \
    debugger \
    fakevim \
#    profilereader \
    aggregation \
    changeset \
#    icheckbuild \
    generichighlighter \
    utils_stringutils

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
