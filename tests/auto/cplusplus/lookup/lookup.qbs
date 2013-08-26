import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus lookup autotest"
    files: "tst_lookup.cpp"
}
