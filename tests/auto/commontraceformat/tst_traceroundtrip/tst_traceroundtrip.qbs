import qbs.FileInfo

QtcAutotest {
    name: "CommonTraceFormat Trace Roundtrip autotest"
    Depends { name: "CommonTraceFormat" }
    files: "tst_traceroundtrip.cpp"
    cpp.defines: base.concat(['TESTDATA_DIR="' + FileInfo.joinPaths(path, '..', 'data') + '"'])
}
