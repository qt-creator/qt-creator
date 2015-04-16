include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelineabstractrenderer \
    timelinemodel \
    timelinemodelaggregator \
    timelinenotesmodel \
    timelineoverviewrenderer \
    timelinerenderer \
    timelinerenderpass \
    timelinerenderstate \
    timelinezoomcontrol

minQtVersion(5,4,0) {
    SUBDIRS += \
        timelineitemsrenderpass \
        timelinenotesrenderpass \
        timelineselectionrenderpass
}

OTHER_FILES += \
    timelineautotest.qbs
