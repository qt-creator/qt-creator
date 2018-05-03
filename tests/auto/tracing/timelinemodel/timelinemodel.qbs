import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodel.cpp"
        ]
    }
}
