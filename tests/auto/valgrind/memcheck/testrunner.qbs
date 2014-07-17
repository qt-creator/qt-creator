import qbs
import "../valgrindautotest.qbs" as ValgrindAutotest

ValgrindAutotest {
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
    files: ["testrunner.h", "testrunner.cpp"]
    destinationDirectory: project.ide_bin_path
    cpp.defines: base.concat([
        'TESTRUNNER_SRC_DIR="' + path + '/testapps"',
        'TESTRUNNER_APP_DIR="' + project.buildDirectory + '/' + destinationDirectory + '/testapps"'
    ])
}
