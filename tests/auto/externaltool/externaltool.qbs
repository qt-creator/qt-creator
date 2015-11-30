import qbs

QtcAutotest {
    name: "ExternalTool autotest"
    Depends { name: "Core" }
    Depends { name: "app_version_header" }

    Group {
        name: "Duplicated sources from Core plugin" // Ewww. But the .pro file does the same.
        prefix: project.ide_source_tree + "/src/plugins/coreplugin/"
        files: [
            "externaltool.cpp",
            "externaltool.h"
        ]
    }

    Group {
        name: "Test sources"
        files: "tst_externaltooltest.cpp"
    }
}
