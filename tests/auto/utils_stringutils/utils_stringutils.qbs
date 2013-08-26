import qbs
import "../autotest.qbs" as Autotest

Autotest {
    name: "StringUtils autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.widgets" } // TODO: Remove when qbs bug is fixed
    files: "tst_stringutils.cpp"
}
