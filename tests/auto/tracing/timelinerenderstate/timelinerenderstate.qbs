import qbs
import QtcFunctions
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineRenderState autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderstate.cpp"
        ]
    }
}
