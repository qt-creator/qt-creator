import qbs

QtcAutotest {
    name: "QtProfiler QML recording timeline autotest"
    cpp.includePaths: base.concat([path + "/../../../../src/plugins/profiler"])
    files: [
        "tst_qmlrecordingtimeline.cpp",
        "../../../../src/plugins/profiler/qmlrecordingtimeline.h",
    ]
}
