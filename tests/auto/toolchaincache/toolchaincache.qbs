import qbs

QtcAutotest {
    name: "ToolChainCache autotest"
    Depends { name: "ProjectExplorer" }
    Depends { name: "Qt.gui" } // For QIcon in Task
    Group {
        name: "Test sources"
        files: "tst_toolchaincache.cpp"
    }
}
