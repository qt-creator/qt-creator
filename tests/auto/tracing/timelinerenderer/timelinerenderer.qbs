import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TrackPainterInteraction autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_trackpainterinteraction.cpp" ]
    }
}
