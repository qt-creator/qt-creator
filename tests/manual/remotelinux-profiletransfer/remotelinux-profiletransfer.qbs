import qbs.FileInfo

QtcManualTest {
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Remote" }
    Depends { name: "Utils" }

    cpp.defines: base.concat('QTC_TEST_LIBEXEC_PATH="'
                             + FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix,
                                                  qtc.ide_libexec_path) + '"')

    files: "tst_sourceprofiletransfer.cpp"
}
