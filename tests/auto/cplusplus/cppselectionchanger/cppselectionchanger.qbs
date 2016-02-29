import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus selection changer autotest"

    Group {
        name: "Source Files"
        files: "tst_cppselectionchangertest.cpp"
    }

    Group {
        name: "Data Files"
        fileTags: ["data"]
        files: [
            "testCppFile.cpp",
        ]
    }

    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
