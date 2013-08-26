import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus code formatter autotest"
    files: "tst_codeformatter.cpp"
}
