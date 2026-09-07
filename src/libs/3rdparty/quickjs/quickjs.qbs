QtcLibrary {
    name: "quickjsng"
    type: "staticlibrary"

    useQt: false

    cpp.warningLevel: "none"
    cpp.cLanguageVersion: "c11"

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.defines: "WIN32_LEAN_AND_MEAN"
    }
    Properties {
        condition: !qbs.targetOS.contains("windows") && !qbs.targetOS.contains("darwin")
        cpp.defines: ["_POSIX_C_SOURCE='200112L'", "_DEFAULT_SOURCE"]
    }
    cpp.defines: base

    Group {
        name: "Sources"
        prefix: "src/"

        files: [
            "builtin-array-fromasync.h",
            "builtin-iterator-zip.h",
            "builtin-iterator-zip-keyed.h",
            "cutils.h",
            "dtoa.c",
            "dtoa.h",
            "libregexp-opcode.h",
            "libregexp.c",
            "libregexp.h",
            "libunicode-table.h",
            "libunicode.c",
            "libunicode.h",
            "list.h",
            "quickjs-atom.h",
            "quickjs-c-atomics.h",
            "quickjs-opcode.h",
            "quickjs.c",
            "quickjs.h",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.systemIncludePaths: project.ide_source_tree + "/src/libs/3rdparty/quickjs/src"
    }
}
