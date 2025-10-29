QtcAutotest {
    name: "QtTaskTree autotest"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }
    Depends { name: "QtTaskTree" }

    files: "tst_qttasktree.cpp"
}
