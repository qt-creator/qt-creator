import qbs

QtcAutotest {
    name: "qrc parser autotest"
    Depends { name: "Utils" }
    files: "tst_qrcparser.cpp"
    cpp.defines: base.concat(['TESTSRCDIR="' + path + '"'])
}
