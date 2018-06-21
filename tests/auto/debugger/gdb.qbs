import qbs

QtcAutotest {
    name: "gdb autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.network" } // For QHostAddress
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: "debuggerprotocol.cpp"
    }
    Group {
        name: "Test sources"
        files: "tst_gdb.cpp"
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
