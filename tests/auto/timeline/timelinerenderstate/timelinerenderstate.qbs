import qbs
import QtcFunctions
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineRenderState autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinerenderstate.cpp"
        ]
    }
}
