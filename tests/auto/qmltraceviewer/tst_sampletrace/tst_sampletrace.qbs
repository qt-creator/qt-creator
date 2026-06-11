import qbs

QtcAutotest {
    name: "QmlTraceViewer sample trace autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Utils" }
    cpp.includePaths: base.concat([path + "/../../../../src/plugins/profiler"])
    files: [
        "tst_sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
