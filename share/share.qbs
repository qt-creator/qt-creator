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
            "android/**/*",
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
            "scripts/**/*",
            "snippets/**/*",
            "styles/**/*",
            "templates/**/*",
            "themes/**/*",
            "welcomescreen/**/*"
        ]
        excludeFiles: [
            "qml-type-descriptions/qbs-bundle.json",
            "qml-type-descriptions/qbs.qmltypes",
        ]
    }

    Group {
        name: "3rdparty"
        qbs.install: true
        qbs.installDir: qtc.ide_data_path
        qbs.installSourceBase: project.ide_source_tree + "/src/share/3rdparty"
        prefix: project.ide_source_tree + "/src/share/3rdparty/"
        files: [
            "fonts/**/*"
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
