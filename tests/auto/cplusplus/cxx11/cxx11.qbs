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
        files: "*"
    }

    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
