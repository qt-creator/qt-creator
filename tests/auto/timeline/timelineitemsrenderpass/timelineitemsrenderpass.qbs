import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineItemsRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineitemsrenderpass.cpp"
        ]
    }
}
