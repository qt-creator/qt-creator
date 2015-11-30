import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    condition: QtcFunctions.versionIsAtLeast(Qt.core.version, "5.4")
    name: "TimelineNotesRenderPass autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesrenderpass.cpp"
        ]
    }
}
