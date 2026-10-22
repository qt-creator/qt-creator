import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineDetailsWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_timelinedetailswidget.cpp" ]
    }
}
