import qbs

QtcAutotest {
    name: "ToolChainCache autotest"
    Depends { name: "ProjectExplorer" }
    Group {
        name: "Test sources"
        files: "tst_toolchaincache.cpp"
    }
}
