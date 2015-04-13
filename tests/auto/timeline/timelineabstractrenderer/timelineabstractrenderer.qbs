import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineAbstractRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineabstractrenderer.cpp"
        ]
    }
}
