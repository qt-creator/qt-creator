import qbs

QtcManualtest {
    name: "Manual QtcProcess test"
    condition: qbs.targetOS.contains("unix")
    Depends { name: "Utils" }
    targetName: "process"

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h"
    ]
}
