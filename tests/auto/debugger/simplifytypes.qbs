import qbs

QtcAutotest {
    name: "simplifytypes autotest"
    Depends { name: "Qt.network" } // For QHostAddress
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: "simplifytype.cpp"
    }
    Group {
        name: "Test sources"
        files: "tst_simplifytypes.cpp"
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
