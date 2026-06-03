import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TrackPainterInteraction autotest"
    Group {
        name: "Test sources"
        files: [ "tst_trackpainterinteraction.cpp" ]
    }
}
