QtcTool {
    name: "qtc-askpass"
    Depends { name: "Qt.network" }
    Depends { name: "Qt.widgets" }
    Qt.core.useRPaths: true
    files: "qtc-askpass-main.cpp"
}
