import qbs 1.0

QtApplication {
    name : "Qt Essential Includes"

    Depends {
        name: "Qt"
        submodules: [
            "network",
            "qml",
            "quick",
            "sql",
            "testlib",
            "widgets",
        ]
    }

    Depends { name: "Qt.multimedia"; required: false }
    Depends { name: "Qt.multimediawidgets"; required: false }

    files : [
        "main.cpp",
    ]
}
