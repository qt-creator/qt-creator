import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderer.cpp"
        ]
    }
}
