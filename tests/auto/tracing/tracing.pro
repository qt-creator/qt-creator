include(../../../qtcreator.pri)
TEMPLATE = subdirs

SUBDIRS = \
    flamegraph \
    flamegraphview \
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
    tracingautotest.qbs
