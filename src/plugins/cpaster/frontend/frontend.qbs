import qbs 1.0

QtcTool {
    name: "cpaster"

    Depends {
        name: "Qt"
        submodules: ["gui", "network"]
    }
    Depends { name: "Core" }
    Depends { name: "CppTools" }

    cpp.rpaths: [
        "$ORIGIN/../" + project.libDirName + "/qtcreator",
        "$ORIGIN/../" + project.libDirName + "/qtcreator/plugins",
    ]

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
            "pastebindotcaprotocol.h", "pastebindotcaprotocol.cpp",
            "pastebindotcomprotocol.h", "pastebindotcomprotocol.cpp",
            "protocol.h", "protocol.cpp",
            "urlopenprotocol.h", "urlopenprotocol.cpp",
        ]
    }
}
