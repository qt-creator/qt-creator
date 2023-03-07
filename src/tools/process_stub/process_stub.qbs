import qbs 1.0

QtcTool {
    name: "qtcreator_process_stub"
    consoleApplication: true

    Depends { name: "Qt"; submodules: ["core", "network"]; }

    files: [ "main.cpp" ]
}
