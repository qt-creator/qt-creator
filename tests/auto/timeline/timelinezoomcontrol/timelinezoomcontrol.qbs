import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineZoomcontrol autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinezoomcontrol.cpp"
        ]
    }
}
