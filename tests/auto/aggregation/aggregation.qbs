import qbs
import QtcAutotest

QtcAutotest {
    name: "Aggregation autotest"
    Depends { name: "Aggregation" }
    files: "tst_aggregate.cpp"
}
