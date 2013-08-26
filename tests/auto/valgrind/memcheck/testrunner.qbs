import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "Memcheck test runner"
    Depends { name: "Memcheck free1 autotest" }
    Depends { name: "Memcheck free2 autotest" }
    Depends { name: "Memcheck invalidjump autotest" }
    Depends { name: "Memcheck leak1 autotest" }
    Depends { name: "Memcheck leak2 autotest" }
    Depends { name: "Memcheck leak3 autotest" }
    Depends { name: "Memcheck leak4 autotest" }
    Depends { name: "Memcheck overlap autotest" }
    Depends { name: "Memcheck syscall autotest" }
    Depends { name: "Memcheck uninit1 autotest" }
    Depends { name: "Memcheck uninit2 autotest" }
    Depends { name: "Memcheck uninit3 autotest" }
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    Depends { name: "Valgrind" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: ["testrunner.h", "testrunner.cpp"]
    destinationDirectory: project.ide_bin_path
    cpp.defines: base.concat([
        'TESTRUNNER_SRC_DIR="' + path + '/testapps"',
        'TESTRUNNER_APP_DIR="' + product.buildDirectory + '/' + destinationDirectory + '/testapps"'
    ])
}
