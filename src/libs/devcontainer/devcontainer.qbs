QtcLibrary {
    name: "DevContainer"
    Depends { name: "Utils" }
    Depends { name: "QtTaskTree" }
    Depends { name: "Qt.core" }

    cpp.defines: base.concat([
        "DEVCONTAINER_LIBRARY"
    ])

    files: [
        "devcontainer_global.h",
        "devcontainer.cpp",
        "devcontainer.h",
        "devcontainerconfig.cpp",
        "devcontainerconfig.h",
        "devcontainertr.h",
        "substitute.cpp",
        "substitute.h"
    ]

    Group {
        name: "JSON schema"
        files: [ "devcontainer.schema.json" ]
    }
}
