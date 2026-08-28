import qbs 1.0

QtcPlugin {
    name: "QDoc"

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }
    Depends { name: "Utils" }
    Depends { name: "Qt"; submodules: ["widgets"] }

    files: [
        "qdocconfig.cpp",
        "qdocconfig.h",
        "qdoceditor.cpp",
        "qdoceditor.h",
        "qdoclinter.cpp",
        "qdoclinter.h",
        "qdocparser.cpp",
        "qdocparser.h",
        "qdocplugin.cpp",
        "qdocrenderer.cpp",
        "qdocrenderer.h",
        "qdoctr.h",
    ]

    Group {
        name: "long description"
        files: "QDocDescription.md"
        fileTags: "pluginjson.longDescription"
    }
}
