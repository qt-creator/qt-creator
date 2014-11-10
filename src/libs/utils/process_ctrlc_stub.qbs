import qbs 1.0

QtcTool {
    name: "qtcreator_ctrlc_stub"
    consoleApplication: true
    condition: qbs.targetOS.contains("windows")


    files: [ "process_ctrlc_stub.cpp" ]

    cpp.dynamicLibraries: [
        "user32",
        "shell32"
    ]
}
