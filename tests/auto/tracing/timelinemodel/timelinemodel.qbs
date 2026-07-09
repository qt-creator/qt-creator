import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineModel autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodel.cpp"
        ]
    }
}
