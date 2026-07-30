import qbs

QtcAutotest {
    name: "Aggregation autotest"
    Depends { name: "Utils" }
    files: "tst_aggregate.cpp"
}
