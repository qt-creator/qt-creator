import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "OverviewWidget autotest"
    Group {
        name: "Test sources"
        files: [ "tst_overviewwidget.cpp" ]
    }
}
