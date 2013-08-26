import qbs
import "../cplusplusautotest.qbs" as CPlusPlusAutotest

CPlusPlusAutotest {
    name: "CPlusPlus AST autotest"
    files: "tst_ast.cpp"
}
