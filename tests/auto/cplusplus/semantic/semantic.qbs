import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus semantic autotest"
    files: "tst_semantic.cpp"
}
