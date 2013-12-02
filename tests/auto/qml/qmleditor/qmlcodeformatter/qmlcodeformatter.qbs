import qbs
import "../../../autotest.qbs" as Autotest

Autotest {
    name: "QML code formatter autotest"
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Qt.widgets" }  // TODO: Remove when qbs bug is fixed
    files: "tst_qmlcodeformatter.cpp"
}
