import qbs

QtcAutotest {
    name: "QML code model ecmascript7 autotest"
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }

    files: "tst_ecmascript7.cpp"

    cpp.defines: base.concat([
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
