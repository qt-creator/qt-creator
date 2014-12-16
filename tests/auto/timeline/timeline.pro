include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelinemodel \
    timelineabstractrenderer

minQtVersion(5,4,0) {
    SUBDIRS += \
        timelineitemsrenderpass
}
