import qbs.FileInfo

QtcAutotest {
    name: "FakeVim autotest"

    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" }

    property string fakeVimDir: FileInfo.joinPaths(project.ide_source_tree, "src", "plugins", "fakevim")

    cpp.defines: base.concat(["FAKEVIM_STANDALONE"])
    cpp.includePaths: base.concat([fakeVimDir])

    files: "tst_fakevim.cpp"

    Group {
        name: "FakeVim files"
        prefix: fakeVimDir + "/"
        files: [
            "fakevimactions.cpp",
            "fakevimactions.h",
            "fakevimhandler.cpp",
            "fakevimhandler.h",
        ]
    }
}
