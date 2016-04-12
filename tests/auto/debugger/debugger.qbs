import qbs

Project {
    name: "Debugger autotests"
    property path debuggerDir: project.ide_source_tree + "/src/plugins/debugger/"
    references: [
        "disassembler.qbs",
        "dumpers.qbs",
        "gdb.qbs",
        "namedemangler.qbs",
        "offsets.qbs",
        "simplifytypes.qbs",
    ]
}
