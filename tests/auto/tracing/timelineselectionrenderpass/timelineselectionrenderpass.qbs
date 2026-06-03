import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "SelectionStatePaint autotest"
    Group {
        name: "Test sources"
        files: [ "tst_selectionstatepaint.cpp" ]
    }
}
