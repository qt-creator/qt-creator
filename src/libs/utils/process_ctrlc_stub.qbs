import qbs.base 1.0
import "../../tools/QtcTool.qbs" as QtcTool

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
