import qbs.FileInfo

QtcAutotest {
    name: "CommonTraceFormat Tracedata autotest"
    Depends { name: "CommonTraceFormat" }
    files: "tst_tracedata.cpp"
    cpp.defines: base.concat(['TESTDATA_DIR="' + FileInfo.joinPaths(path, '..', 'data') + '"'])
}
