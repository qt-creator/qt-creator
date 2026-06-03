import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "FlameGraphWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_flamegraphwidget.cpp" ]
    }
}
