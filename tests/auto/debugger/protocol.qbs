import qbs

QtcAutotest {
    name: "debugger protocol autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.network" } // For QHostAddress
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: ["debuggerprotocol.cpp", "debuggertr.h"]
    }
    Group {
        name: "Test sources"
        files: "tst_protocol.cpp"
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
