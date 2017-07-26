import qbs
import "../../autotest.qbs" as Autotest

Autotest {
    name: "CamelHumpMatcher autotest"
    Depends { name: "Utils" }
    files: "tst_camelhumpmatcher.cpp"
}
