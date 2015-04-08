include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelineabstractrenderer \
    timelinemodel \
    timelinemodelaggregator

minQtVersion(5,4,0) {
    SUBDIRS += \
        timelineitemsrenderpass
}
