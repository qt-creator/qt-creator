import qbs 1.0

QtcLibrary {
    name: "ptracepreload"
    condition: qbs.targetOS.contains("linux")
    install: false

    cpp.dynamicLibraries: [
        "dl",
        "c"
    ]


    files: [
        "ptracepreload.c",
    ]
}
