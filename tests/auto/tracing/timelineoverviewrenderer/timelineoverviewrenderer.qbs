import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "OverviewWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_overviewwidget.cpp" ]
    }
}
