TEMPLATE = subdirs

SUBDIRS += \
    cplusplus \
    debugger \
    fakevim \
#    profilereader \
    aggregation \
    changeset \
#    icheckbuild \
    generichighlighter

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
