import qbs 1.0

QtApplication {
    name : "Qt Widgets Application"

    Depends {
        name: "Qt"
        submodules: [ "widgets" ]
    }

    files : [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.ui"
    ]
}
