import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineOverviewRenderer autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineoverviewrenderer.cpp"
        ]
    }
}
