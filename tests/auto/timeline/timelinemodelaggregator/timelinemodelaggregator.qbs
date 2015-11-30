import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineModelAggregator autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodelaggregator.cpp"
        ]
    }
}
