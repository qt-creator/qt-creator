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
    qml \
    utils_stringutils

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
