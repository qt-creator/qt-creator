import qbs

QtcAutotest {
    name: "ToolChainCache autotest"
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt"; submodules: ["gui", "widgets"] } // For QIcon in Task, QLabel in ElidingLabel
    Group {
        name: "Test sources"
        files: "tst_toolchaincache.cpp"
    }
}
