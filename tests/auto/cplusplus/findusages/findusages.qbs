import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus find usages autotest"
    files: [ "tst_findusages.cpp", "../cplusplus_global.h" ]
}
