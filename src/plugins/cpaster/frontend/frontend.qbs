import qbs.base 1.0
import "../../../tools/QtcTool.qbs" as QtcTool

QtcTool {
    name: "cpaster"

    Depends { name: "cpp" }
    Depends {
        name: "Qt"
        submodules: "core", "gui", "network"
    }
    Depends { name: "Core" }

    cpp.includePaths: ["../../"]
    cpp.rpaths: [
        "$ORIGIN/../lib/qtcreator",
        "$ORIGIN/../lib/qtcreator/plugins",
        "$ORIGIN/../lib/qtcreator/plugins/Nokia"
    ]

    files: [ "main.cpp",
        "argumentscollector.h", "argumentscollector.cpp",
        "../cpasterconstants.h",
        "../protocol.h", "../protocol.cpp",
        "../pastebindotcomprotocol.h", "../pastebindotcomprotocol.cpp",
        "../pastebindotcaprotocol.h", "../pastebindotcaprotocol.cpp",
        "../kdepasteprotocol.h", "../kdepasteprotocol.cpp",
        "../urlopenprotocol.h", "../urlopenprotocol.cpp"
    ]
}
