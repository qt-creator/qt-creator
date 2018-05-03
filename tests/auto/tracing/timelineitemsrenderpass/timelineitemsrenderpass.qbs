import qbs
import QtcFunctions
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineItemsRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineitemsrenderpass.cpp"
        ]
    }
}
