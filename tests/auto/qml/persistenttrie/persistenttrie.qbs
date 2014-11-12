import qbs

QtcAutotest {
    name: "QML persistenttrie autotest"
    Depends { name: "QmlJS" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: [ "tst_testtrie.h", "tst_testtrie.cpp" ]
    cpp.defines: base.concat([
        'QMLJS_BUILD_DIR',
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
