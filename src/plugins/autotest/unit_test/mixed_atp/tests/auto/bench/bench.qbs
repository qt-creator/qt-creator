import qbs

CppApplication {
    type: "application"
    name: "Benchmark Auto Test"
    targetName: "tst_benchtest"

    Depends { name: "cpp" }
    Depends { name: "Qt.testlib" }

    files: [ "tst_benchtest.cpp" ]

    cpp.defines: base.concat("SRCDIR=" + path)
}
