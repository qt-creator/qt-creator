import qbs

QtcAutotest {
    name: "QML persistenttrie autotest"
    Depends { name: "QmlJS" }
    files: [ "tst_testtrie.h", "tst_testtrie.cpp" ]
    cpp.defines: base.concat([
        'QMLJS_LIBRARY',
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
