QtcAutotest {
    name: "Debugger pdb dumper autotest"
    Group {
        name: "Test sources"
        files: "tst_pdb.cpp"
    }
    cpp.defines: base.concat([
        'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
        'PDBDUMPER_DRIVER="' + path + '/pdbdumper.py"'
    ])
}
