import qbs
import QtcFunctions
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineSelectionRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineselectionrenderpass.cpp"
        ]
    }
}
