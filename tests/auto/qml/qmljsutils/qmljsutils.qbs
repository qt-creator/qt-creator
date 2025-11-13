import qbs

QtcAutotest {
    name: "QMLJS utils autotest"
    Depends { name: "QmlJS" }
    files: "tst_qmljsutils.cpp"
}
