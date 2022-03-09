import qbs.FileInfo

QtcAutotest {
    name: "QtcProcess autotest"
    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    files: "tst_qtcprocess.cpp"
    cpp.defines: {
        var defines = base;
        if (qbs.targetOS === "windows")
            defines.push("_CRT_SECURE_NO_WARNINGS");
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }
}
