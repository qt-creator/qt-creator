import qbs

QtcAutotest {
    name: "QML qrc parser autotest"
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    files: "tst_qrcparser.cpp"
    cpp.defines: base.concat(['TESTSRCDIR="' + path + '"'])
}
