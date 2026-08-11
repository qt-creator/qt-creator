import qbs.FileInfo

QtcAutotest {
    name: "CmdBridge autotest"

    Depends { name: "app_version_header" }
    Depends { name: "CmdBridgeClient" }
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        defines.push('TEST_LIBEXEC_PATH="' + absLibExecPath + '"');
        return defines;
    }

    files: "tst_cmdbridge.cpp"
}
