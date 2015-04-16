import qbs

Project {
    name: "Timeline autotests"

    property path timelineDir: project.ide_source_tree
            + "/src/libs/timeline"
    references: [
        "timelinemodel/timelinemodel.qbs",
        "timelinemodelaggregator/timelinemodelaggregator.qbs",
        "timelinenotesmodel/timelinenotesmodel.qbs",
        "timelineabstractrenderer/timelineabstractrenderer.qbs",
        "timelineitemsrenderpass/timelineitemsrenderpass.qbs",
        "timelinenotesrenderpass/timelinenotesrenderpass.qbs",
        "timelineoverviewrenderer/timelineoverviewrenderer.qbs",
        "timelinerenderer/timelinerenderer.qbs",
        "timelinerenderpass/timelinerenderpass.qbs",
        "timelinerenderstate/timelinerenderstate.qbs",
        "timelineselectionrenderpass/timelineselectionrenderpass.qbs",
        "timelinezoomcontrol/timelinezoomcontrol.qbs"
    ]
}
