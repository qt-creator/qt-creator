import qbs.base 1.0
import "../../../tools/QtcTool.qbs" as QtcTool

QtcTool {
    name: "cpaster"

    Depends {
        name: "Qt"
        submodules: ["gui", "network"]
    }
    Depends { name: "Core" }

    cpp.includePaths: ["../../"]
    cpp.rpaths: [
        "$ORIGIN/../lib/qtcreator",
        "$ORIGIN/../lib/qtcreator/plugins",
        "$ORIGIN/../lib/qtcreator/plugins/QtProject"
    ]

    files: [ "main.cpp",
        "argumentscollector.h", "argumentscollector.cpp",
        "../cpasterconstants.h",
        "../kdepasteprotocol.h", "../kdepasteprotocol.cpp",
        "../pastebindotcaprotocol.h", "../pastebindotcaprotocol.cpp",
        "../pastebindotcomprotocol.h", "../pastebindotcomprotocol.cpp",
        "../protocol.h", "../protocol.cpp",
        "../urlopenprotocol.h", "../urlopenprotocol.cpp",
    ]
}
