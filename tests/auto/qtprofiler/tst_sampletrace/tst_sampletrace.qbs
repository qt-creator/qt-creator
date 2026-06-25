import qbs

QtcAutotest {
    name: "QtProfiler sample trace autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Utils" }
    cpp.includePaths: base.concat([path + "/../../../../src/plugins/profiler"])
    cpp.defines: base.concat( ["QMLPROFILER_STATIC_LIBRARY"] )
    files: [
        "tst_sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
