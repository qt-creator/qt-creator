import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineSelectionRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelineselectionrenderpass.cpp"
        ]
    }
}
