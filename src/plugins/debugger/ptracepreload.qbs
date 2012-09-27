import qbs.base 1.0
import "../../libs/QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "ptracepreload"
    condition: qbs.targetOS == "linux"

    cpp.dynamicLibraries: [
        "dl",
        "c"
    ]

    Depends { name: "cpp" }

    files: [
        "ptracepreload.c",
    ]
}
