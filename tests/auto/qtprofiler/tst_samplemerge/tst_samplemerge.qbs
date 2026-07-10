import qbs

QtcAutotest {
    name: "QtProfiler sample merge autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Utils" }
    cpp.includePaths: base.concat([
        path + "/../../../../src/plugins/profiler",
        path + "/../../../../src/plugins",
    ])
    files: [
        "tst_samplemerge.cpp",
        "../../../../src/plugins/profiler/samplemerge.cpp",
        "../../../../src/plugins/profiler/samplemerge.h",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
