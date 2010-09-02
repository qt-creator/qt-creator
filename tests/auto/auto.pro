TEMPLATE = subdirs

SUBDIRS += \
    cplusplus \
    debugger \
    fakevim \
#    profilereader \
#    profilewriter \
    aggregation \
    changeset \
#    icheckbuild \
    generichighlighter \
    utils_stringutils \
    qml

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
