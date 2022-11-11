import qbs.FileInfo

QtcManualtest {
    name: "Manual TaskTree test"
    type: ["application"]

    Depends { name: "Utils" }

    files: [
        "main.cpp",
        "taskwidget.h",
        "taskwidget.cpp",
    ]
}
