import qbs

QtcAutotest {
    name: "QtcProcess autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: qbs bug, remove when fixed
    files: "tst_qtcprocess.cpp"
    Properties {
        condition: qbs.targetOS === "windows"
        cpp.defines: base.concat(["_CRT_SECURE_NO_WARNINGS"])
    }
}
