import qbs
import "../autotest.qbs" as Autotest

Autotest {
    name: "Aggregation autotest"
    Depends { name: "Aggregation" }
    files: "tst_aggregate.cpp"
}
