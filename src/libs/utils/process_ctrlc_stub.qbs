import qbs.base 1.0
import "../../tools/QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcreator_ctrlc_stub"
    consoleApplication: true
    condition: qbs.targetOS == "windows"

    Depends { name: "cpp" }

    files: [ "process_ctrlc_stub.cpp" ]

    cpp.dynamicLibraries: [
        "user32",
        "shell32"
    ]
}
