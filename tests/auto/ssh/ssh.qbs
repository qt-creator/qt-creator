import qbs.FileInfo

QtcAutotest {
    name: "SSH autotest"
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }
    files: "tst_ssh.cpp"
    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        defines.push("WITH_TESTS");
        return defines;
    }
}
