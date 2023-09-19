import qbs.FileInfo

QtcManualTest {
    name: "Tasking demo"
    type: ["application"]

    Depends { name: "Qt"; submodules: ["network", "widgets"] }
    Depends { name: "Tasking" }

    files: [
        "demo.qrc",
        "main.cpp",
        "progressindicator.h",
        "progressindicator.cpp",
        "taskwidget.h",
        "taskwidget.cpp",
    ]
}
