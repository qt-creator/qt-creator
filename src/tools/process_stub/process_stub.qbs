import qbs 1.0

QtcTool {
    name: "qtcreator_process_stub"
    windowsFileDescription: qtc.ide_display_name + " Process Stub"
    consoleApplication: true

    Depends { name: "Qt.network" }

    files: "main.cpp"
}
