import qbs
import "../clangstaticanalyzerautotest.qbs" as ClangStaticAnalyzerAutotest

ClangStaticAnalyzerAutotest {
    name: "ClangStaticAnalyzerRunner Autotest"

    Group {
        name: "sources from plugin"
        prefix: pluginDir + '/'
        files: [
            "clangstaticanalyzerrunner.cpp",
            "clangstaticanalyzerrunner.h",
        ]
    }

    files: [
        "tst_clangstaticanalyzerrunner.cpp",
    ]
}
