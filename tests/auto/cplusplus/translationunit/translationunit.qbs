import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus translation unit autotest"
    files: [ "tst_translationunit.cpp", "../cplusplus_global.h" ]
}
