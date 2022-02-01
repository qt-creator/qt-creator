import qbs
import qbs.FileInfo

QtcManualtest {
    name: "Manual sftpfs model test"
    condition: qbs.targetOS.contains("unix")
    Depends { name: "Utils" }
    Depends { name: "QtcSsh" }
    Depends { name: "Qt.widgets" }

    cpp.includePaths: [ "../../../../src/shared/modeltest" ]

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }

    files: [
        "main.cpp",
        "window.cpp",
        "window.h",
        "window.ui",
    ]

    Group {
        name: "Model test files"
        prefix: "../../../../src/shared/modeltest/"

        files: [
            "modeltest.cpp",
            "modeltest.h"
        ]
    }
}
