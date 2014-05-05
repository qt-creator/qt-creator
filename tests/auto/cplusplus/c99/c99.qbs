import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "C99 autotest"
    Group {
        name: "Source Files"
        files: "tst_c99.cpp"
    }

    Group {
        name: "Data Files"
        prefix: "data/"
        fileTags: ["data"]
        files: [
            "designatedInitializer.1.c",
        ]
    }

    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
