import qbs.base 1.0

Product {
    type: ["installed_content"]
    name: "SharedContent"

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

    Group {
        condition: qbs.targetOS == "macx"
        qbs.installDir: "share/qtcreator/scripts"
        fileTags: ["install"]
        files: "qtcreator/scripts/openTerminal.command"
    }

    Group {
        qbs.installDir: "share/qtcreator/externaltools"
        fileTags: ["install"]
        prefix: "../src/share/qtcreator/externaltools/"
        files: {
            var list = [
                "lrelease.xml",
                "lupdate.xml",
                "qmlviewer.xml",
                "sort.xml",
            ]
            switch (qbs.targetOS) {
            case "windows":
                list.push("notepad_win.xml");
                break;
            case "mac":
                list.push("vi_mac.xml");
                break;
            default:
                list.push("vi.xml");
            }
            return list;
        }
    }
}

