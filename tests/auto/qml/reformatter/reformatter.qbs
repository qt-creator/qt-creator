import qbs

QtcAutotest {
    name: "QML reformatter autotest"
    Depends { name: "QmlJS" }
    Depends { name: "Qt.widgets" }
    files: "tst_reformatter.cpp"
    cpp.defines: base.concat([
        'QT_CREATOR',
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
