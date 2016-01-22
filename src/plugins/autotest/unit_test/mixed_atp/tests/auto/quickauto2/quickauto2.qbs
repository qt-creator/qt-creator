import qbs

CppApplication {
    name: "Qt Quick auto test 2"
    targetName: "test_mal_qtquick"

    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends {
        condition: Qt.core.versionMajor > 4
        name: "Qt.qmltest"
    }

    Group {
        condition: Qt.core.versionMajor > 4
        name: "main application"
        files: [ "main.cpp" ]
    }

    Group {
        name: "qml test files"
        qbs.install: true

        files: [ "tst_test1.qml", "tst_test2.qml" ]
    }

    // this should be set automatically, but it seems as if this does not happen
    cpp.defines: base.concat("QUICK_TEST_SOURCE_DIR=\"" + path + "\"")
}
