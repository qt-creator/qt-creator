import qbs.base 1.0

Product {
    name: "SharedContent"

    Group {
        name: "Unconditional"
        qbs.install: true
        qbs.installDir: project.ide_data_path
        prefix: "qtcreator/"
        files: [
            "designer",
            "dumper",
            "generic-highlighter",
            "glsl",
            "qml",
            "qmldesigner",
            "qmlicons",
            "qml-type-descriptions",
            "schemes",
            "snippets",
            "styles",
            "templates",
            "welcomescreen"
        ]
    }

    Group {
        name: "Conditional"
        qbs.install: true
        qbs.installDir: project.ide_data_path + "/externaltools"
        prefix: "../src/share/qtcreator/externaltools/"
        files: {
            var list = [
                "lrelease.xml",
                "lupdate.xml",
                "qmlscene.xml",
                "qmlviewer.xml",
                "sort.xml",
            ]
            if (qbs.targetOS.contains("windows"))
                list.push("notepad_win.xml");
            else if (qbs.targetOS.contains("osx"))
                list.push("vi_mac.xml");
            else
                list.push("vi.xml");
            return list;
        }
    }
}
