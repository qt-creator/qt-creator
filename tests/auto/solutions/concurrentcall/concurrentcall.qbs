QtcAutotest {
    name: "ConcurrentCall autotest"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }
    Depends { name: "Tasking" }

    files: "tst_concurrentcall.cpp"
}
