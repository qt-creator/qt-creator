import qbs.base 1.0
import "../../libs/QtcLibrary.qbs" as QtcLibrary

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
