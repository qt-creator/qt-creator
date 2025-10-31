QtcAutotest {
    name: "ConcurrentCall autotest"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }
    Depends { name: "QtTaskTree" }

    files: "tst_concurrentcall.cpp"
}
