QtcAutotest {
    name: "Tasking autotest"

    Depends { name: "Qt"; submodules: ["network"] }
    Depends { name: "Tasking" }

    files: "tst_tasking.cpp"
}
