import qbs.FileInfo

QtcManualTest {
    name: "DeviceShell manualtest"

    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }

    files: "tst_deviceshell.cpp"
}
