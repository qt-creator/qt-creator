import qbs

QtcAutotest {
    name: "TreeModel autotest"
    Depends { name: "Utils" }
    files: "tst_treemodel.cpp"
}
