import qbs
import "../clangtoolsautotest.qbs" as ClangToolsAutotest

ClangToolsAutotest {
    name: "ClangStaticAnalyzerRunner Autotest"

    Group {
        name: "sources from plugin"
        prefix: pluginDir + '/'
        files: [
            "clangstaticanalyzerrunner.cpp",
            "clangstaticanalyzerrunner.h",
            "clangtoolrunner.cpp",
            "clangtoolrunner.h",
        ]
    }

    files: [
        "tst_clangstaticanalyzerrunner.cpp",
    ]
}
