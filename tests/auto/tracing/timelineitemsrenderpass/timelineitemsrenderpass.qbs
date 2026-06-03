import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TrackPainterVisibility autotest"
    Group {
        name: "Test sources"
        files: [ "tst_trackpaintervisibility.cpp" ]
    }
}
