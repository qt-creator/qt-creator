import qbs.FileInfo

QtcAutotest {
    name: "sdktool autotest"

    Group {
        name: "Test sources"
        files: "tst_sdktool.cpp"
    }

    cpp.defines: base.concat([
        'SDKTOOL_DIR="' + FileInfo.joinPaths(FileInfo.fromNativeSeparators(qbs.installRoot),
                                             qbs.installPrefix, qtc.ide_libexec_path) + '"'
    ])
}
