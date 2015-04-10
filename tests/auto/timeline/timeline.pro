include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelineabstractrenderer \
    timelinemodel \
    timelinemodelaggregator \
    timelinenotesmodel

minQtVersion(5,4,0) {
    SUBDIRS += \
        timelineitemsrenderpass \
        timelinenotesrenderpass
}

OTHER_FILES += \
    timelineautotest.qbs
