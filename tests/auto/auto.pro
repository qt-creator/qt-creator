TEMPLATE = subdirs

SUBDIRS += \
    cplusplus \
    debugger \
    fakevim \
#    profilereader \
    aggregation \
    changeset \
	icheckbuild

contains (QT_CONFIG, declarative) {
SUBDIRS += qml
}
