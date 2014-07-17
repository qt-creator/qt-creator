import qbs
import "../valgrindautotest.qbs" as ValgrindAutotest

ValgrindAutotest {
    name: "Memcheck parser autotest"
    Depends { name: "valgrind-fake" }
    Depends { name: "Qt.network" }
    files: ["parsertests.h", "parsertests.cpp"]
    cpp.defines: base.concat([
        'PARSERTESTS_DATA_DIR="' + path + '/data"',
        'VALGRIND_FAKE_PATH="' + project.buildDirectory + '/' + project.ide_bin_path + '/valgrind-fake"'
    ])
}
