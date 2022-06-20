import qbs
import qbs.FileInfo
import "../valgrindautotest.qbs" as ValgrindAutotest

ValgrindAutotest {
    name: "Memcheck ModelDemo autotest"
    Depends { name: "valgrind-fake" }
    Depends { name: "Qt.network" }
    files: ["modeldemo.h", "modeldemo.cpp"]
    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        defines.push('PARSERTESTS_DATA_DIR="' + path + '/data"');
        defines.push('VALGRIND_FAKE_PATH="' + project.buildDirectory + '/' + qtc.ide_bin_path + '/valgrind-fake"');
        return defines;
    }
}
