import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "SharedConditionally"

    Group {
        qbs.installDir: "share/qtcreator/externaltools"
        fileTags: ["install"]
        prefix: "qtcreator/externaltools/"
        files: [
            "lrelease.xml",
            "lupdate.xml",
            "qmlviewer.xml",
            "qmlscene.xml",
            "sort.xml",
        ]
    }

    Group {
        condition: qbs.targetOS == "linux"
        qbs.installDir: "share/qtcreator/externaltools"
        fileTags: ["install"]
        files: ["qtcreator/externaltools/vi.xml"]
    }

    Group {
        condition: qbs.targetOS == "mac"
        qbs.installDir: "share/qtcreator/externaltools"
        fileTags: ["install"]
        files: ["qtcreator/externaltools/vi_mac.xml"]
    }

    Group {
        condition: qbs.targetOS == "windows"
        qbs.installDir: "share/qtcreator/externaltools"
        fileTags: ["install"]
        files: ["qtcreator/externaltools/notepad_win.xml"]
    }
}

