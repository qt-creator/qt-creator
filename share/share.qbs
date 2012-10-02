import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "SharedContent"

    Group {
        condition: qbs.targetOS == "macx"
        qbs.installDir: "share/qtcreator/scripts"
        fileTags: ["install"]
        files: "qtcreator/scripts/openTerminal.command"
    }

    Group {
        qbs.installDir: "share"
        fileTags: ["install"]
        files: "qtcreator"
        recursive: true
        excludeFiles: [
            "qtcreator/translations",
            "qtcreator/scripts",
            "share.pro",
            "share.qbs",
            "static.pro",
        ]
    }
}

