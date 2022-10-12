import qbs.FileInfo

Project  {
    QtcAutotest {
        name: "TaskTree autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "tst_tasktree.cpp",
        ]
        cpp.defines: {
            var defines = base;
            if (qbs.targetOS === "windows")
                defines.push("_CRT_SECURE_NO_WARNINGS");
            var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                    qtc.ide_libexec_path);
            var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
            defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
            defines.push('TESTAPP_PATH="'
                         + FileInfo.joinPaths(destinationDirectory, "testapp") + '"');
            return defines;
        }
    }
    references: "testapp/testapp.qbs"
}
