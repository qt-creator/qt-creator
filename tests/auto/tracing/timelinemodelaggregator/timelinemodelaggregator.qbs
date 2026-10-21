import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineModelAggregator autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [
            "tst_timelinemodelaggregator.cpp"
        ]
    }
}
