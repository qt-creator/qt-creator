import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TrackPainterProperties autotest"
    Group {
        name: "Test sources"
        files: [ "tst_trackpainterproperties.cpp" ]
    }
}
