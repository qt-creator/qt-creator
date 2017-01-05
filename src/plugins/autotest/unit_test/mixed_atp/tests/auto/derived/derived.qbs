import qbs

CppApplication {
    type: "application"
    name: "Derived Auto Test"
    targetName: "tst_derivedtest"

    Depends { name: "cpp" }
    Depends { name: "Qt.testlib" }

    files: [ "origin.cpp", "origin.h", "tst_derivedtest.cpp" ]

    cpp.defines: base.concat("SRCDIR=" + path)
}
