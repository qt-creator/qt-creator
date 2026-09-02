import qbs

StaticLibrary {
    name: "inlibtests"

    Depends { name: "cpp" }
    Depends { name: "Qt.testlib" }

    files: [ "tst_inlib.cpp", "tst_inlib.h" ]

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt.testlib" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
