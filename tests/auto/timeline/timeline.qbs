import qbs

Project {
    name: "Timeline autotests"

    property path timelineDir: project.ide_source_tree
            + "/src/libs/timeline"
    references: [
        "timelinemodel/timelinemodel.qbs",
        "timelinemodelaggregator/timelinemodelaggregator.qbs",
        "timelineabstractrenderer/timelineabstractrenderer.qbs",
        "timelineitemsrenderpass/timelineitemsrenderpass.qbs",
    ]
}
