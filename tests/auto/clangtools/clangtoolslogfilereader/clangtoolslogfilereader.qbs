import qbs
import "../clangtoolsautotest.qbs" as ClangToolsAutotest

ClangToolsAutotest {
    name: "ClangToolsLogFileReader Autotest"

    Depends { name: "libclang"; required: false }

    cpp.defines: base.concat('SRCDIR="' + sourceDirectory + '"')

    condition: libclang.present

    cpp.includePaths: base.concat(libclang.llvmIncludeDir)
    cpp.libraryPaths: base.concat(libclang.llvmLibDir)
    cpp.dynamicLibraries: base.concat(libclang.llvmLibs)
    cpp.rpaths: base.concat(libclang.llvmLibDir)

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
