import qbs

Project {
    name: "ClangStaticAnalyzer autotests"
    references: [
        "clangstaticanalyzerlogfilereader",
        "clangstaticanalyzerrunner",
    ]
}
