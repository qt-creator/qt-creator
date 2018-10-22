import qbs 1.0

QtcTool {
    name: "cpaster"

    Depends {
        name: "Qt"
        submodules: ["gui", "network"]
    }
    Depends { name: "Core" }
    Depends { name: "CppTools" }

    Group {
        name: "Frontend Sources"
        files: [
            "main.cpp",
            "argumentscollector.h", "argumentscollector.cpp"
        ]
    }

    Group {
        name: "Plugin Sources"
        prefix: "../"
        files: [
            "cpasterconstants.h",
            "kdepasteprotocol.h", "kdepasteprotocol.cpp",
            "pastebindotcomprotocol.h", "pastebindotcomprotocol.cpp",
            "pastecodedotxyzprotocol.h", "pastecodedotxyzprotocol.cpp",
            "protocol.h", "protocol.cpp",
            "urlopenprotocol.h", "urlopenprotocol.cpp",
        ]
    }
}
