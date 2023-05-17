QtcAutotest {
    name: "Tasking autotest"

    Depends { name: "Tasking" }
    Depends { name: "Utils" }

    files: "tst_tasking.cpp"
}
