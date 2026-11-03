import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineOverviewWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_timelineoverviewwidget.cpp" ]
    }
}
