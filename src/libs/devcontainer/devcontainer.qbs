QtcLibrary {
    name: "DevContainer"
    Depends { name: "Utils" }
    Depends { name: "Tasking" }
    Depends { name: "Qt.core" }

    files: [
        "devcontainer_global.h",
        "devcontainer.cpp",
        "devcontainer.h",
        "devcontainerconfig.cpp",
        "devcontainerconfig.h",
        "devcontainertr.h",
    ]

    Group {
        name: "JSON schema"
        files: [ "devcontainer.schema.json" ]
    }
}
