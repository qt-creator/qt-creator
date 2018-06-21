import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineZoomcontrol autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinezoomcontrol.cpp"
        ]
    }
}
