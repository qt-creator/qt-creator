import qbs

QtcAutotest {
    name: "disassembler autotest"
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtcSsh" }
    Group {
        name: "Sources from Debugger plugin"
        prefix: project.debuggerDir
        files: "disassemblerlines.cpp"
    }
    Group {
        name: "Test sources"
        files: "tst_disassembler.cpp"
    }
    cpp.includePaths: base.concat([project.debuggerDir])
}
