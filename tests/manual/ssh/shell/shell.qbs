import qbs

QtcManualtest {
    name: "Manual ssh shell test"
    condition: qbs.targetOS.contains("unix")
    Depends { name: "Utils" }
    Depends { name: "QtcSsh" }
    Depends { name: "Qt.network" }

    files: [
        "argumentscollector.cpp",
        "argumentscollector.h",
        "main.cpp",
        "shell.cpp",
        "shell.h",
    ]
}
