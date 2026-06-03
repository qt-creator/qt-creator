import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "TimelineModelState autotest"
    Group {
        name: "Test sources"
        files: [ "tst_timelinemodelstate.cpp" ]
    }
}
