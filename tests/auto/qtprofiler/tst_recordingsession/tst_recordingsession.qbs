import qbs

QtcAutotest {
    name: "QtProfiler recording session autotest"
    Depends { name: "Utils" }
    cpp.includePaths: base.concat([path + "/../../../../src/plugins/profiler"])
    cpp.defines: base.concat( ["PROFILER_STATIC_LIBRARY"] )
    files: [
        "tst_recordingsession.cpp",
        "../../../../src/plugins/profiler/sampler.cpp",
        "../../../../src/plugins/profiler/samplerrecipe.cpp",
        "../../../../src/plugins/profiler/sampler.h",
    ]
}
