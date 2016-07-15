import qbs

QtcAutotest {
    name: "QML code model dependencies autotest"
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }

    files: "tst_dependencies.cpp"

    cpp.defines: base.concat([
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
