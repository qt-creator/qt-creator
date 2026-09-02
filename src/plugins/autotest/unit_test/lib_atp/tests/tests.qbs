import qbs

CppApplication {
    type: "application"
    name: "Library auto test"
    targetName: "tst_libtest"

    Depends { name: "inlibtests" }

    files: [ "main.cpp" ]
}
