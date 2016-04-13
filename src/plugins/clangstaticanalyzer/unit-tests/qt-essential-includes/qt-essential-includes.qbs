import qbs 1.0

QtApplication {
    name : "Qt Essential Includes"

    Depends {
        name: "Qt"
        submodules: [
            "multimedia",
            "multimediawidgets",
            "network",
            "qml",
            "quick",
            "sql",
            "testlib",
            "widgets",
        ]
    }

    files : [
        "main.cpp",
    ]
}
