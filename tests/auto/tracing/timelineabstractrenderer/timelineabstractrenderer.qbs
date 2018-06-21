import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineAbstractRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineabstractrenderer.cpp"
        ]
    }
}
