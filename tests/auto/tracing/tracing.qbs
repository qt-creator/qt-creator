import qbs

Project {
    name: "Tracing autotests"

    references: [
        "flamegraph/flamegraph.qbs",
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
