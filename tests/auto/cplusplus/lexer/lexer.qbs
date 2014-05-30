import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus lexer autotest"
    files: [ "tst_lexer.cpp", "../cplusplus_global.h" ]
}
