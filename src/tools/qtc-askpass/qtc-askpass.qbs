QtcTool {
    name: "qtc-askpass"
    windowsFileDescription: qtc.ide_display_name + " Askpass Helper"
    Depends { name: "Qt.network" }
    Depends { name: "Qt.widgets" }
    Qt.core.useRPaths: true
    files: "qtc-askpass-main.cpp"
}
