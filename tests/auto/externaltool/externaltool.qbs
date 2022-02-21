import qbs

QtcAutotest {
    name: "ExternalTool autotest"
    Depends { name: "Core" }
    Depends { name: "app_version_header" }

    Group {
        name: "Test sources"
        files: "tst_externaltooltest.cpp"
    }
}
