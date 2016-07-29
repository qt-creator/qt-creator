import qbs 1.0

Product {
    name: "SharedContent"
    Depends { name: "qtc" }

    Group {
        name: "Unconditional"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path
        qbs.installSourceBase: "qtcreator"
        prefix: "qtcreator/"
        files: [
            "cplusplus/**/*",
            "debugger/**/*",
            "designer/**/*",
            "glsl/**/*",
            "modeleditor/**/*",
            "qml/**/*",
            "qmldesigner/**/*",
            "qmlicons/**/*",
            "qml-type-descriptions/**/*",
            "schemes/**/*",
            "snippets/**/*",
            "styles/**/*",
            "templates/**/*",
            "themes/**/*",
            "welcomescreen/**/*"
        ]
    }

    Group {
        name: "3rdparty"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path
        qbs.installSourceBase: project.ide_source_tree + "/src/share/3rdparty"
        prefix: project.ide_source_tree + "/src/share/3rdparty/"
        files: [
            "generic-highlighter/**/*"
        ]
    }

    Group {
        name: "Conditional"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path + "/externaltools"
        prefix: project.ide_source_tree + "/src/share/qtcreator/externaltools/"
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
            else if (qbs.targetOS.contains("macos"))
                list.push("vi_mac.xml");
            else
                list.push("vi.xml");
            return list;
        }
    }
}
