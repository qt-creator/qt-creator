import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "FlameGraph autotest"
    Group {
        name: "Test sources"
        files: [
            "tst_flamegraph.cpp"
        ]
    }
}
