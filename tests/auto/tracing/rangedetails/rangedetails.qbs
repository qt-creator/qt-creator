import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "RangeDetailsWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_rangedetailswidget.cpp" ]
    }
}
