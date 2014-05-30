import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus check symbols autotest"
    files: [ "tst_checksymbols.cpp", "../cplusplus_global.h" ]
}
