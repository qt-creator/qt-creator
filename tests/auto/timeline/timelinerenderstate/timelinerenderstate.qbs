import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderpass.cpp"
        ]
    }
}
