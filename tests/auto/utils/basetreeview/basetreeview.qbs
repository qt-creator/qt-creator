import qbs

QtcAutotest {
    name: "BaseTreeView autotest"
    Depends { name: "Utils" }
    files: "tst_basetreeview.cpp"
}
