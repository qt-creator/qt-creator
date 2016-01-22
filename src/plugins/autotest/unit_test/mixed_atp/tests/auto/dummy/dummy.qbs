import qbs

CppApplication {
    type: "application"
    name: "Dummy auto test"
    targetName: "tst_FooBar"

    Depends { name: "Qt.test" }
    Depends { name: "Qt.gui" }

    files: [ "tst_foo.cpp", "tst_foo.h" ]
}
