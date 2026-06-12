import qbs

QtcAutotest {
    name: "QmlTraceViewer call tree model autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Utils" }
    cpp.includePaths: base.concat([
        path + "/../../../../src/plugins/profiler",
        path + "/../../../../src/plugins",
    ])
    files: [
        "tst_calltreemodel.cpp",
        "../../../../src/plugins/profiler/calltreemodel.cpp",
        "../../../../src/plugins/profiler/calltreemodel.h",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
