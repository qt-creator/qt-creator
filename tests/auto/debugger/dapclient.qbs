import qbs.FileInfo

QtcAutotest {
    name: "Debugger DAP client autotest"
    Depends { name: "Utils" }
    Group {
        name: "Test sources"
        files: "tst_dapclient.cpp"
    }
    Group {
        name: "Sources under test"
        prefix: FileInfo.joinPaths(project.debuggerDir, "dap/")
        files: ["dapclient.cpp", "dapclient.h"]
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
