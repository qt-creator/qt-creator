import qbs 1.0
import QtcLibrary

QtcLibrary {
    name: "ptracepreload"
    condition: qbs.targetOS.contains("linux")

    cpp.dynamicLibraries: [
        "dl",
        "c"
    ]


    files: [
        "ptracepreload.c",
    ]
}
