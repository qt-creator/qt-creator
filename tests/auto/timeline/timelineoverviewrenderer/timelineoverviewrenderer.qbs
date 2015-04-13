import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineOverviewRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineoverviewrenderer.cpp"
        ]
    }
}
