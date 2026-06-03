import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "FlameGraphWidgetView autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_flamegraphwidgetview.cpp" ]
    }
}
