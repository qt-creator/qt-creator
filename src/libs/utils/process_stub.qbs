import qbs.base 1.0
import "../../tools/QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcreator_process_stub"
    consoleApplication: true

    Depends { name: "cpp" }

    files: {
        if (qbs.targetOS == "windows") {
            return [ "process_stub_win.c" ]
        } else {
            return [ "process_stub_unix.c" ]
        }
    }

    cpp.dynamicLibraries: {
        if (qbs.targetOS == "windows") {
            return [ "shell32" ]
        }
    }
}
