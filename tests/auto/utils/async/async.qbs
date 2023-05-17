import qbs

QtcAutotest {
    name: "Async autotest"
    Depends { name: "Tasking" }
    Depends { name: "Utils" }
    files: "tst_async.cpp"
}
