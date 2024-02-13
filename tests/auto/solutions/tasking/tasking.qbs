QtcAutotest {
    name: "Tasking autotest"

    Depends { name: "Qt"; submodules: ["concurrent", "network"] }
    Depends { name: "Tasking" }

    files: "tst_tasking.cpp"
}
