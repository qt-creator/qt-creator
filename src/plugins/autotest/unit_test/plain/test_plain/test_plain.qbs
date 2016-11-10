import qbs

CppApplication {
    type: "application" // suppress bundle generation on OSX

    Depends { name: "Qt.gui" }
    Depends { name: "Qt.testlib" }

    files: [ "tst_simple.cpp", "tst_simple.h" ]
}
