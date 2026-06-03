import qbs
import "../tracingautotest.qbs" as TracingAutotest

TracingAutotest {
    name: "NotesModelFiltering autotest"
    Group {
        name: "Test sources"
        files: [ "tst_notesmodelfiltering.cpp" ]
    }
}
