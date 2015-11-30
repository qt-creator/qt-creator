import qbs

QtcAutotest {
    name: "Aggregation autotest"
    Depends { name: "Aggregation" }
    files: "tst_aggregate.cpp"
}
