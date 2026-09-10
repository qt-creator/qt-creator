import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineContentWidget autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: [ "tst_timelinecontentwidget.cpp" ]
    }
}
