import qbs 1.0

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
