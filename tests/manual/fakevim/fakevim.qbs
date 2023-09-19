import qbs.FileInfo

QtcManualTest {
    name: "Manual FakeVim test"
    type: ["application"]

    Depends { name: "Utils" }

    property string fakeVimDir: FileInfo.joinPaths(project.ide_source_tree, "src", "plugins", "fakevim")

    cpp.defines: base.concat(["FAKEVIM_STANDALONE"])
    cpp.includePaths: fakeVimDir

    files: "main.cpp"

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
