import qbs
import "../valgrindautotest.qbs" as ValgrindAutotest

ValgrindAutotest {
    name: "Memcheck ModelDemo autotest"
    Depends { name: "valgrind-fake" }
    Depends { name: "Qt.network" }
    files: ["modeldemo.h", "modeldemo.cpp"]
    cpp.defines: base.concat([
        'PARSERTESTS_DATA_DIR="' + path + '/data"',
        'VALGRIND_FAKE_PATH="' + project.buildDirectory + '/' + qtc.ide_bin_path + '/valgrind-fake"'
    ])
}
