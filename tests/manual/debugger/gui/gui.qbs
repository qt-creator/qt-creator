QtApplication {
    name: "Manual debugger gui test"

    Depends { name: "Qt.widgets" }

    files: [
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.ui",
        "tst_gui.cpp",
    ]

    install: false
}
