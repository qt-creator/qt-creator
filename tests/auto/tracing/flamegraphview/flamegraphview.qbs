import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "FlameGraphView autotest"

    Depends { name: "Qt.quickwidgets" }

    Group {
        name: "Test sources"
        files: [
            "tst_flamegraphview.cpp", "flamegraphview.qrc"
        ]
    }
}
