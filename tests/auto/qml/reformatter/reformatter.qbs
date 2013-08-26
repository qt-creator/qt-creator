import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "QML reformatter autotest"
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_reformatter.cpp"
    cpp.defines: base.concat([
        'QT_CREATOR',
        'QTCREATORDIR="' + project.ide_source_tree + '"',
        'TESTSRCDIR="' + path + '"'
    ])
}
