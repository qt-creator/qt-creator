TEMPLATE = subdirs

SUBDIRS += \
    cplusplus \
    debugger \
    fakevim \
#    profilereader \
    aggregation \
    changeset

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
