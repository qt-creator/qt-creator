import qbs

QtcAutotest {
    name: "QMLJS simple reader autotest"
    Depends { name: "QmlJS" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_qmljssimplereader.cpp"
    cpp.defines: base.concat(["QT_CREATOR"])
}
