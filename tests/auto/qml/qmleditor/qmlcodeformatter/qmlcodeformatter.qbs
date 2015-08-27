import qbs

QtcAutotest {
    name: "QML code formatter autotest"
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Qt.widgets" }
    files: "tst_qmlcodeformatter.cpp"
}
