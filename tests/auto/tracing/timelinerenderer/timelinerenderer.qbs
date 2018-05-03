import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderer.cpp"
        ]
    }
}
