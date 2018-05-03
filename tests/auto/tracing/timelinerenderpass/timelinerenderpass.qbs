import qbs
import QtcFunctions
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderpass.cpp"
        ]
    }
}
