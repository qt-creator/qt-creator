import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineCoordinates autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinecoordinates.cpp"
        ]
    }
}
