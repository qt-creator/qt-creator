import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus pretty printer autotest"
    files: "tst_typeprettyprinter.cpp"
}
