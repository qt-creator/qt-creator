import qbs

Project {
    name: "Debugger autotests"
    property path debuggerDir: project.ide_source_tree + "/src/plugins/debugger/"
    references: [
        "dumpers.qbs",
        "gdb.qbs",
        "namedemangler.qbs",
        "simplifytypes.qbs",
        "disassembler.qbs"
    ]
}
