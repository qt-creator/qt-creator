import qbs.FileInfo

QtcManualTest {
    name: "CmdBridge manualtest"

    Depends { name: "app_version_header" }
    Depends { name: "CmdBridgeClient" }
    Depends { name: "Utils" }

    cpp.defines: {
        var defines = base;
        var absLibExecPath = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                qtc.ide_libexec_path);
        defines.push('TEST_LIBEXEC_PATH="' + absLibExecPath + '"');
        return defines;
    }

    files: "tst_cmdbridge.cpp"
}
