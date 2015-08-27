import qbs

QtcAutotest {
    name: "QMLJS simple reader autotest"
    Depends { name: "QmlJS" }
    files: "tst_qmljssimplereader.cpp"
    cpp.defines: base.concat(["QT_CREATOR"])
}
