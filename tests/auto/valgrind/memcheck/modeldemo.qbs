import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "Memcheck ModelDemo autotest"
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    Depends { name: "Valgrind" }
    Depends { name: "valgrind-fake" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    Depends { name: "Qt.network" }
    files: ["modeldemo.h", "modeldemo.cpp"]
    cpp.defines: base.concat([
        'PARSERTESTS_DATA_DIR="' + path + '/data"',
        'VALGRIND_FAKE_PATH="' + product.buildDirectory + '/' + project.ide_bin_path + '/valgrind-fake"'
    ])
}
