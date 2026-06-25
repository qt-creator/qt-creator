import qbs

Project {
    name: "QtProfiler autotests"
    references: [
        "tst_calltreemodel/tst_calltreemodel.qbs",
        "tst_cpuusagemodel/tst_cpuusagemodel.qbs",
        "tst_sampletrace/tst_sampletrace.qbs",
        "tst_symbolicator/tst_symbolicator.qbs",
    ]
}
