QtcLibrary {
    name: "quickjs"
    type: "staticlibrary"

    cpp.warningLevel: "none"

    Group {
        name: "Sources"
        prefix: "src/"

        files: [
            "builtin-array-fromasync.h",
            "cutils.c",
            "cutils.h",
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
            "xsum.c",
            "xsum.h",
        ]
    }

    Export {
        cpp.includePaths: project.ide_source_tree + "/src/libs/3rdparty/quickjs/src"
    }
}
