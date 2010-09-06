TEMPLATE = subdirs

SUBDIRS += \
    aggregation \
    changeset \
    cplusplus \
    debugger \
    fakevim \
    generichighlighter \
#    icheckbuild \
#    profilereader \
#    profilewriter \
    qml \
    utils_stringutils

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
