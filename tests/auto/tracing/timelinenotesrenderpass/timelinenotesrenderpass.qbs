import qbs
import QtcFunctions
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineNotesRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesrenderpass.cpp"
        ]
    }
}
