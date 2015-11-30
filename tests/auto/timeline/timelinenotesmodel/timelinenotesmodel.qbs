import qbs
import "../timelineautotest.qbs" as TimelineAutotest

TimelineAutotest {
    name: "TimelineNotesModel autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_timelinenotesmodel.cpp"
        ]
    }
}
