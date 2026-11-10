import qbs

QtcAutotest {
    name: "QtProfiler call stack lane model autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Tracing"; required: false }
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }
    condition: Tracing.present
    cpp.defines: base.concat(["PROFILER_STATIC_LIBRARY"])
    cpp.includePaths: base.concat([
        path + "/../../../../src/plugins/profiler",
        path + "/../../../../src/plugins",
    ])
    files: [
        "tst_callstacklanemodel.cpp",
        "../../../../src/plugins/profiler/callstacklanemodel.cpp",
        "../../../../src/plugins/profiler/callstacklanemodel.h",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
