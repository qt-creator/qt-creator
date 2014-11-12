import qbs 1.0

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
