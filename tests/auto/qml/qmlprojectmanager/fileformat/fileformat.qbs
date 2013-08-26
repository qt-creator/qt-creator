import qbs
import "../../../autotest.qbs" as Autotest

Autotest {
    condition: false
    name: "QmlProjectManager file format autotest"
    Depends { name: "QmlProjectManager" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["script", "declarative"]; }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_fileformat.cpp"
    cpp.includePaths: base.concat([project.ide_source_tree + "/src/plugins/qmlprojectmanager/fileformat"])
    cpp.defines: base.concat(['SRCDIR="' + path + '"'])
}
