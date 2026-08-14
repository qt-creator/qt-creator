QtcAutotest {
    name: "Debugger bridge autotest"
    Group {
        name: "Test sources"
        files: "tst_bridge.cpp"
    }
    cpp.defines: base.concat([
        'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
        'BRIDGE_DRIVER="' + path + '/bridgedriver.py"'
    ])
}
