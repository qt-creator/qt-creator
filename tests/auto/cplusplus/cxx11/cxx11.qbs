import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "Cxx11 autotest"
    Group {
        name: "Source Files"
        files: "tst_cxx11.cpp"
    }

    Group {
        name: "Data Files"
        prefix: "data/"
        fileTags: ["data"]
        files: [
            "inlineNamespace.1.cpp",
            "inlineNamespace.1.errors.txt",
            "staticAssert.1.cpp",
            "staticAssert.1.errors.txt",
            "noExcept.1.cpp",
            "noExcept.1.errors.txt"
        ]
    }

    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
