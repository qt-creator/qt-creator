include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelineabstractrenderer \
    timelinemodel \
    timelinemodelaggregator \
    timelinenotesmodel \
    timelineoverviewrenderer \
    timelinerenderer \
    timelinerenderpass

minQtVersion(5,4,0) {
    SUBDIRS += \
        timelineitemsrenderpass \
        timelinenotesrenderpass
}

OTHER_FILES += \
    timelineautotest.qbs
