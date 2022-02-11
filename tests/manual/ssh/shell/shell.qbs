import qbs
import qbs.FileInfo

QtcManualtest {
    name: "Manual ssh shell test"
    condition: qbs.targetOS.contains("unix")
    Depends { name: "Utils" }
    Depends { name: "QtcSsh" }
    Depends { name: "Qt.network" }

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }

    files: [
        "argumentscollector.cpp",
        "argumentscollector.h",
        "main.cpp",
        "shell.cpp",
        "shell.h",
    ]
}
