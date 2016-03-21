import qbs

CppApplication {
    name: "Qt Quick auto test"
    targetName: "test_mal_qtquick"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.qmltest" }

    Group {
        name: "main application"

        files: [ "main.cpp" ]
    }

    Group {
        name: "qml test files"

        files: [
            "tst_test1.qml", "tst_test2.qml", "TestDummy.qml",
            "bar/tst_foo.qml", "tst_test3.qml"
        ]
    }

    // this should be set automatically, but it seems as if this does not happen
    cpp.defines: base.concat("QUICK_TEST_SOURCE_DIR=\"" + path + "\"")
}
