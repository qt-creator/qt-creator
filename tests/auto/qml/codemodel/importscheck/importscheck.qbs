import qbs

QtcAutotest {
    name: "QML code model imports check autotest"
    Depends { name: "LanguageUtils" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }
    files: "tst_importscheck.cpp"
    cpp.defines: base.concat([
        'QT_CREATOR',
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
