import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineNotesModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesmodel.cpp"
        ]
    }
}
