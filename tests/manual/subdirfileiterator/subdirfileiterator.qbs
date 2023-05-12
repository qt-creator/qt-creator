import qbs.FileInfo

QtcManualtest {
    name: "Manual SubDirFileIterator test"
    type: ["application"]

    Depends { name: "Utils" }
    Depends { name: "app_version_header" }

    files: [
        "tst_subdirfileiterator.cpp",
    ]

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        var relLibExecPath = FileInfo.relativePath(destinationDirectory, absLibExecPath);
        defines.push('TEST_RELATIVE_LIBEXEC_PATH="' + relLibExecPath + '"');
        return defines;
    }
}
