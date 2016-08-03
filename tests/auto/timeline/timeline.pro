include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    timelineabstractrenderer \
    timelineitemsrenderpass \
    timelinemodel \
    timelinemodelaggregator \
    timelinenotesmodel \
    timelinenotesrenderpass \
    timelineoverviewrenderer \
    timelinerenderer \
    timelinerenderpass \
    timelinerenderstate \
    timelineselectionrenderpass \
    timelinezoomcontrol

OTHER_FILES += \
    timelineautotest.qbs
