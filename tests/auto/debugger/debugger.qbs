import qbs

Project {
    name: "Debugger autotests"
    property path debuggerDir: project.ide_source_tree + "/src/plugins/debugger/"
    references: [
        "disassembler.qbs",
        "dumpers.qbs",
        "gdb.qbs",
        "protocol.qbs",
        "offsets.qbs",
        "simplifytypes.qbs",
    ]
}
