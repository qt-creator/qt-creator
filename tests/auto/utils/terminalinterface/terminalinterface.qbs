import qbs.FileInfo

Project  {
    QtcAutotest {
        name: "TerminalInterface autotest"

        Depends { name: "Utils" }
        Depends { name: "app_version_header" }
        Depends { name: "Qt.network" }

        files: [
            "tst_terminalinterface.cpp",
        ]
        cpp.defines: {
            var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                    qtc.ide_libexec_path);
            var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
            return base.concat('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        }
    }
}
