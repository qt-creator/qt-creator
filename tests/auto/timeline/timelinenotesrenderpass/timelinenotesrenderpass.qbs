import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineNotesRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesrenderpass.cpp"
        ]
    }
}
