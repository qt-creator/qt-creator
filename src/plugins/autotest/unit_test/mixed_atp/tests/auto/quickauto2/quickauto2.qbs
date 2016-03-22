import qbs

CppApplication {
    name: "Qt Quick auto test 2"
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

        files: [ "tst_test1.qml", "tst_test2.qml" ]
    }

    // this should be set automatically, but it seems as if this does not happen
    cpp.defines: base.concat("QUICK_TEST_SOURCE_DIR=\"" + path + "\"")
}
