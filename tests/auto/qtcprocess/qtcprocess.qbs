import qbs

QtcAutotest {
    name: "QtcProcess autotest"
    Depends { name: "Utils" }
    files: "tst_qtcprocess.cpp"
    Properties {
        condition: qbs.targetOS === "windows"
        cpp.defines: base.concat(["_CRT_SECURE_NO_WARNINGS"])
    }
}
