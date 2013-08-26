import qbs
import "../autotest.qbs" as Autotest

Autotest {
    name: "gdb autotest"
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
