import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "Callgrind parser autotest"
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    Depends { name: "Valgrind" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    property path pluginDir: project.ide_source_tree + "/src/plugins/valgrind"
    files: ["callgrindparsertests.h", "callgrindparsertests.cpp"]
    cpp.defines: base.concat([
        'CALLGRINDPARSERTESTS',
        'PARSERTESTS_DATA_DIR="' + path + '/data"'
    ])
}
