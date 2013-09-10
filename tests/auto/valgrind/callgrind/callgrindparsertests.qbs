import qbs
import "../valgrindautotest.qbs" as ValgrindAutotest

ValgrindAutotest {
    name: "Callgrind parser autotest"
    files: ["callgrindparsertests.h", "callgrindparsertests.cpp"]

    cpp.defines: base.concat([
        'CALLGRINDPARSERTESTS',
        'PARSERTESTS_DATA_DIR="' + path + '/data"'
    ])
}
