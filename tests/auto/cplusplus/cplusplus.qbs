import qbs

Project {
    name: "CPlusPlus autotests"
    references: [
        "ast/ast.qbs",
        "c99/c99.qbs",
        "checksymbols/checksymbols.qbs",
        "codeformatter/codeformatter.qbs",
        "cppselectionchanger/cppselectionchanger.qbs",
        "cxx11/cxx11.qbs",
        "fileiterationorder/fileiterationorder.qbs",
        "findusages/findusages.qbs",
        "lexer/lexer.qbs",
        "lookup/lookup.qbs",
        "misc/misc.qbs",
        "preprocessor/preprocessor.qbs",
        "semantic/semantic.qbs",
        "translationunit/translationunit.qbs",
        "typeprettyprinter/typeprettyprinter.qbs"
    ]
}
