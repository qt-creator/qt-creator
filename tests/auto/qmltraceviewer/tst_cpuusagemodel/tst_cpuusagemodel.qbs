import qbs

QtcAutotest {
    name: "QmlTraceViewer CPU usage model autotest"
    Depends { name: "CommonTraceFormat" }
    Depends { name: "Tracing" }
    Depends { name: "Utils" }
    Depends { name: "Qt.gui" }
    cpp.includePaths: base.concat([
        path + "/../../../../src/plugins/profiler",
        path + "/../../../../src/plugins",
    ])
    files: [
        "tst_cpuusagemodel.cpp",
        "../../../../src/plugins/profiler/cpuusagemodel.cpp",
        "../../../../src/plugins/profiler/cpuusagemodel.h",
        "../../../../src/plugins/profiler/sampletrace.cpp",
        "../../../../src/plugins/profiler/sampletrace.h",
    ]
}
