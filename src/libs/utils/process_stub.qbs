import qbs.base 1.0
import "../../tools/QtcTool.qbs" as QtcTool

QtcTool {
    name: "qtcreator_process_stub"
    consoleApplication: true


    files: {
        if (qbs.targetOS.contains("windows")) {
            return [ "process_stub_win.c" ]
        } else {
            return [ "process_stub_unix.c" ]
        }
    }

    cpp.dynamicLibraries: {
        if (qbs.targetOS.contains("windows")) {
            return [ "shell32" ]
        }
    }
}
