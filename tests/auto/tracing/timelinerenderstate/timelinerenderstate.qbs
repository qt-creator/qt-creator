import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "ZoomControlState autotest"
    Group {
        name: "Test sources"
        files: [ "tst_zoomcontrolstate.cpp" ]
    }
}
