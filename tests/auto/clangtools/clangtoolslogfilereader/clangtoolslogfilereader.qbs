import qbs
import "../clangtoolsautotest.qbs" as ClangToolsAutotest

ClangToolsAutotest {
    name: "ClangToolsLogFileReader Autotest"
    cpp.defines: base.concat('SRCDIR="' + sourceDirectory + '"')

    Group {
        name: "sources from plugin"
        prefix: pluginDir + '/'
        files: [
            "clangtoolsdiagnostic.cpp",
            "clangtoolsdiagnostic.h",
            "clangtoolslogfilereader.cpp",
            "clangtoolslogfilereader.h",
        ]
    }

    files: [
        "tst_clangtoolslogfilereader.cpp"
    ]
}
