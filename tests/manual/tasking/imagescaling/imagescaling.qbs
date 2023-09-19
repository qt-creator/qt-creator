QtcManualTest {
    name: "Tasking imagescaling"
    type: ["application"]

    Depends { name: "Qt"; submodules: ["concurrent", "network", "widgets"] }
    Depends { name: "Tasking" }

    files: [
        "downloaddialog.cpp",
        "downloaddialog.h",
        "downloaddialog.ui",
        "imagescaling.cpp",
        "imagescaling.h",
        "main.cpp",
    ]
}
