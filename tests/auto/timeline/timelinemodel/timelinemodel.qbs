import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodel.cpp"
        ]
    }
}
