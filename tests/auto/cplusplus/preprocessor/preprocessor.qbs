import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus preprocessor autotest"
    Group {
        name: "Source Files"
        files: "tst_preprocessor.cpp"
    }

    Group {
        name: "Data files"
        prefix: "data/"
        fileTags: ["data"]
        files: [
            "empty-macro.cpp", "empty-macro.out.cpp",
            "empty-macro.2.cpp", "empty-macro.2.out.cpp",
            "identifier-expansion.1.cpp", "identifier-expansion.1.out.cpp",
            "identifier-expansion.2.cpp", "identifier-expansion.2.out.cpp",
            "identifier-expansion.3.cpp", "identifier-expansion.3.out.cpp",
            "identifier-expansion.4.cpp", "identifier-expansion.4.out.cpp",
            "identifier-expansion.5.cpp", "identifier-expansion.5.out.cpp",
            "macro_expand.c", "macro_expand.out.c",
            "macro_expand_1.cpp", "macro_expand_1.out.cpp",
            "macro-test.cpp", "macro-test.out.cpp",
            "macro_pounder_fn.c",
            "noPP.1.cpp",
            "noPP.2.cpp",
            "poundpound.1.cpp", "poundpound.1.out.cpp",
            "recursive.1.cpp", "recursive.1.out.cpp",
            "reserved.1.cpp", "reserved.1.out.cpp",
        ]
    }

    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
